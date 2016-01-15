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

/*
 * Note:
 * This is a workaround for the following Qt5 bugs.
 *
 * #49947: Drop events have broken mimeData()->urls() and text/uri-list.
 * #47981: Qt5.4 regression: Dropping text/urilist over browser windows stop working.
 *
 * Related LXQt bug: https://github.com/lxde/lxqt/issues/688
 *
 * This workaround is not 100% reliable, but it should work most of the time.
 * In theory, when there are multiple drag and drops at nearly the same time and
 * you are using a remote X11 instance via a slow network connection, this workaround
 * might break. However, that should be a really rare corner case.
 *
 * How this fix works:
 * 1. Hook QApplication to filter raw X11 events
 * 2. Intercept SelectionRequest events sent from XDnd target window.
 * 3. Check if the data requested have the type "text/uri-list" or "x-moz-url"
 * 4. Bypass the broken Qt5 code and send the mime data to the target with our own code.
 *
 * The mime data is obtained during the most recent mouse button release event.
 * This can be incorrect in some corner cases, but it is still a simple and
 * good enough approximation that returns the correct data most of the time.
 * Anyway, a workarond is just a workaround. Ask Qt developers to fix their bugs.
 */

#ifndef XDNDWORKAROUND_H
#define XDNDWORKAROUND_H

#include <QtGlobal>

// This workaround is for Qt >= 5.4
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <xcb/xcb.h>
#include <QByteArray>

// class _QBasicDrag;
class QDrag;

class XDndWorkaround : public QAbstractNativeEventFilter
{
public:
  XDndWorkaround();
  ~XDndWorkaround();
  bool nativeEventFilter(const QByteArray & eventType, void * message, long * result) override;

private:
  bool clientMessage(xcb_client_message_event_t* event);
  bool selectionRequest(xcb_selection_request_event_t* event);
  bool genericEvent(xcb_ge_generic_event_t *event);
  QByteArray atomName(xcb_atom_t atom) const;
  // _QBasicDrag* xcbDrag() const;
  void buttonRelease();

private:
  QDrag* lastDrag_;

  // xinput related
  bool m_xi2Enabled;
  int m_xiOpCode;
  int m_xiEventBase;
  int m_xiErrorBase;
};

#define WORKAROUND_QT5_XDND_BUG(variableName) \
    XDndWorkaround variableName;

#else // older versions of Qt5 do not require the workaround

#define WORKAROUND_QT5_XDND_BUG(variableName) // do nothing for older Qt 5 versions

#endif // Qt version check

#endif // XDNDWORKAROUND_H
