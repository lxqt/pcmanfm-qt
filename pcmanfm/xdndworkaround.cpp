/*
 * Copyright (C) 2016  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <QtGlobal>

// This workaround is for Qt >= 5.4
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))

#include "xdndworkaround.h"
#include <QApplication>
#include <QDebug>
#include <QX11Info>
#include <QMimeData>
#include <QDrag>
#include <QUrl>
#include <string.h>

// these are private Qt headers which are not part of Qt APIs
#include <private/qdnd_p.h>  // Too bad that we need to use private headers of Qt :-(

// #include <private/qguiapplication_p.h>
// #include <private/qsimpledrag_p.h>
// #include <qpa/qplatformintegration.h>
// #include <qpa/qplatformdrag.h>

// For some unknown reasons, the event type constants defined in
// xcb/input.h are different from that in X11/extension/XI2.h
// To be safe, we define it ourselves.
#undef XI_ButtonRelease
#define XI_ButtonRelease                 5

/*
class _QBasicDrag : public QBasicDrag {
public:
  bool canDrop() const {
    return QBasicDrag::canDrop();
  }
};
*/

XDndWorkaround::XDndWorkaround():
  lastDrag_(nullptr) {

  if(!QX11Info::isPlatformX11())
    return;

  // we need to filter all X11 events
  qApp->installNativeEventFilter(this);

  // initialize xinput2 since newer versions of Qt5 uses it.
  static char xi_name[] = "XInputExtension";
  xcb_connection_t* conn = QX11Info::connection();
  xcb_query_extension_cookie_t cookie = xcb_query_extension(conn, strlen(xi_name), xi_name);
  xcb_generic_error_t* err = nullptr;
  xcb_query_extension_reply_t* reply = xcb_query_extension_reply(conn, cookie, &err);
  if(err == nullptr) {
    m_xi2Enabled = true;
    m_xiOpCode = reply->major_opcode;
    m_xiEventBase = reply->first_event;
    m_xiErrorBase = reply->first_error;
    // qDebug() << "xinput: " << m_xi2Enabled << m_xiOpCode << m_xiEventBase;
  }
  else {
    m_xi2Enabled = false;
    free(err);
  }
  free(reply);
}

XDndWorkaround::~XDndWorkaround() {
  if(!QX11Info::isPlatformX11())
    return;
  qApp->removeNativeEventFilter(this);
}

/*
_QBasicDrag* XDndWorkaround::xcbDrag() const {
  QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
  if(pi != nullptr)
      return static_cast<_QBasicDrag*>(pi->drag());
  return nullptr;
}
*/

bool XDndWorkaround::nativeEventFilter(const QByteArray & eventType, void * message, long * result) {
  if(Q_LIKELY(eventType == "xcb_generic_event_t")) {
    xcb_generic_event_t* event = static_cast<xcb_generic_event_t *>(message);
    uint8_t response_type = event->response_type & uint8_t(~0x80);
    switch(event->response_type & ~0x80) {
    case XCB_SELECTION_REQUEST:
      return selectionRequest(reinterpret_cast<xcb_selection_request_event_t*>(event));
    case XCB_CLIENT_MESSAGE:
      return clientMessage(reinterpret_cast<xcb_client_message_event_t*>(event));
    case XCB_GE_GENERIC:
      // newer versions of Qt5 supports xinput2, which sends its mouse events via XGE.
      return genericEvent(reinterpret_cast<xcb_ge_generic_event_t*>(event));
    case XCB_BUTTON_RELEASE:
      // older versions of Qt5 receive mouse events via old XCB events.
      buttonRelease();
      break;
    default:
      break;
    }
  }
  return false;
}

QByteArray XDndWorkaround::atomName(xcb_atom_t atom) const {
  QByteArray name;
  xcb_connection_t* conn = QX11Info::connection();
  xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, atom);
  xcb_get_atom_name_reply_t* reply = xcb_get_atom_name_reply(conn, cookie, NULL);
  int len = xcb_get_atom_name_name_length(reply);
  if(len > 0) {
    name.append(xcb_get_atom_name_name(reply), len);
  }
  free(reply);
  return name;
}

bool XDndWorkaround::clientMessage(xcb_client_message_event_t* event) {
  xcb_connection_t* conn = QX11Info::connection();
  QByteArray event_type = atomName(event->type);
  // qDebug() << "client message:" << event_type;
  if(event_type == "XdndFinished")
    lastDrag_ = nullptr;
  return false;
}

bool XDndWorkaround::selectionRequest(xcb_selection_request_event_t* event) {
  xcb_connection_t* conn = QX11Info::connection();
  if(event->property == XCB_ATOM_PRIMARY || event->property == XCB_ATOM_SECONDARY)
    return false; // we only touch selection requests related to XDnd
  QByteArray prop_name = atomName(event->property);
  if(prop_name == "CLIPBOARD")
    return false; // we do not touch clipboard, either

  xcb_atom_t atomFormat = event->target;
  QByteArray type_name = atomName(atomFormat);
  // qDebug() << "selection request" << prop_name << type_name;
  // We only want to handle text/x-moz-url and text/uri-list
  if(type_name == "text/x-moz-url" || type_name.startsWith("text/uri-list")) {
    QDragManager* mgr = QDragManager::self();
    QDrag* drag = mgr->object();
    if(drag == nullptr)
      drag = lastDrag_;
    QMimeData* mime = drag ? drag->mimeData() : nullptr;
    if(mime != nullptr && mime->hasUrls()) {
      QByteArray data;
      QList<QUrl> uris = mime->urls();
      if(type_name == "text/x-moz-url") {
        QString mozurl = uris.at(0).toString(QUrl::FullyEncoded);
        data.append((const char*)mozurl.utf16(), mozurl.length() * 2);
      }
      else { // text/uri-list
        for(const QUrl& uri : uris) {
          data.append(uri.toString(QUrl::FullyEncoded));
          data.append("\r\n");
        }
      }
      xcb_change_property(conn, XCB_PROP_MODE_REPLACE, event->requestor, event->property,
                          atomFormat, 8, data.size(), (const void*)data.constData());
      xcb_selection_notify_event_t notify;
      notify.response_type = XCB_SELECTION_NOTIFY;
      notify.requestor = event->requestor;
      notify.selection = event->selection;
      notify.time = event->time;
      notify.property = event->property;
      notify.target = atomFormat;
      xcb_window_t proxy_target = event->requestor;
      xcb_send_event(conn, false, proxy_target, XCB_EVENT_MASK_NO_EVENT, (const char *)&notify);
      return true; // stop Qt 5 from touching the event
    }
  }
  return false; // let Qt handle this
}

bool XDndWorkaround::genericEvent(xcb_ge_generic_event_t* event) {
  // check this is an xinput event
  if(m_xi2Enabled && event->extension == m_xiOpCode) {
    if(event->event_type == XI_ButtonRelease)
      buttonRelease();
  }
  return false;
}

void XDndWorkaround::buttonRelease() {
  QDragManager* mgr = QDragManager::self();
  lastDrag_ = mgr->object();
  // qDebug() << "BUTTON RELEASE!!!!" << xcbDrag()->canDrop() << lastDrag_;
}


#endif // QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
