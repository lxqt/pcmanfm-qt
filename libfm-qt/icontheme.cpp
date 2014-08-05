/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "icontheme.h"
#include <libfm/fm.h>
#include <QList>
#include <QIcon>
#include <QtGlobal>
#include <QApplication>
#include <QDesktopWidget>

using namespace Fm;

static IconTheme* theIconTheme = NULL; // the global single instance of IconTheme.
static const char* fallbackNames[] = {"unknown", "application-octet-stream", NULL};

static void fmIconDataDestroy(gpointer data) {
  QIcon* picon = reinterpret_cast<QIcon*>(data);
  delete picon;
}

IconTheme::IconTheme():
  currentThemeName_(QIcon::themeName()) {
  // NOTE: only one instance is allowed
  Q_ASSERT(theIconTheme == NULL);
  Q_ASSERT(qApp != NULL); // QApplication should exists before contructing IconTheme.

  theIconTheme = this;
  fm_icon_set_user_data_destroy(reinterpret_cast<GDestroyNotify>(fmIconDataDestroy));
  fallbackIcon_ = iconFromNames(fallbackNames);

  // We need to get notified when there is a QEvent::StyleChange event so
  // we can check if the current icon theme name is changed.
  // To do this, we can filter QApplication object itself to intercept
  // signals of all widgets, but this may be too inefficient.
  // So, we only filter the events on QDesktopWidget instead.
  qApp->desktop()->installEventFilter(this);
}

IconTheme::~IconTheme() {
}

IconTheme* IconTheme::instance() {
  return theIconTheme;
}

// check if the icon theme name is changed and emit "changed()" signal if any change is detected.
void IconTheme::checkChanged() {
  if(QIcon::themeName() != theIconTheme->currentThemeName_) {
    // if the icon theme is changed
    theIconTheme->currentThemeName_ = QIcon::themeName();
    // invalidate the cached data
    fm_icon_reset_user_data_cache(fm_qdata_id);

    theIconTheme->fallbackIcon_ = iconFromNames(fallbackNames);
    Q_EMIT theIconTheme->changed();
  }
}

QIcon IconTheme::iconFromNames(const char* const* names) {
  const gchar* const* name;
  // qDebug("names: %p", names);
  for(name = names; *name; ++name) {
    // qDebug("icon name=%s", *name);
    QString qname = *name;
    QIcon qicon = QIcon::fromTheme(qname);
    if(!qicon.isNull()) {
      return qicon;
    }
  }
  return QIcon();
}

QIcon IconTheme::convertFromGIcon(GIcon* gicon) {
  if(G_IS_THEMED_ICON(gicon)) {
    const gchar * const * names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
    QIcon icon = iconFromNames(names);
    if(!icon.isNull())
      return icon;
  }
  else if(G_IS_FILE_ICON(gicon)) {
    GFile* file = g_file_icon_get_file(G_FILE_ICON(gicon));
    char* fpath = g_file_get_path(file);
    QString path = fpath;
    g_free(fpath);
    return QIcon(path);
  }
  return theIconTheme->fallbackIcon_;
}


//static
QIcon IconTheme::icon(FmIcon* fmicon) {
  // check if we have a cached version
  QIcon* picon = reinterpret_cast<QIcon*>(fm_icon_get_user_data(fmicon));
  if(!picon) { // we don't have a cache yet
    picon = new QIcon(); // what a waste!
    *picon = convertFromGIcon(G_ICON(fmicon));
    fm_icon_set_user_data(fmicon, picon); // store it in FmIcon
  }
  return *picon;
}

//static
QIcon IconTheme::icon(GIcon* gicon) {
  if(G_IS_THEMED_ICON(gicon)) {
    FmIcon* fmicon = fm_icon_from_gicon(gicon);
    QIcon qicon = icon(fmicon);
    fm_icon_unref(fmicon);
    return qicon;
  }
  else if(G_IS_FILE_ICON(gicon)) {
    // we do not map GFileIcon to FmIcon deliberately.
    return convertFromGIcon(gicon);
  }
  return theIconTheme->fallbackIcon_;
}

// this method is called whenever there is an event on the QDesktopWidget object.
bool IconTheme::eventFilter(QObject* obj, QEvent* event) {
  // we're only interested in the StyleChange event.
  if(event->type() == QEvent::StyleChange) {
    checkChanged(); // check if the icon theme is changed
  }
  return QObject::eventFilter(obj, event);
}
