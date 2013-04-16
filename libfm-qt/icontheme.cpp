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

using namespace Fm;

static IconTheme* theIconTheme = NULL; // the global single instance of IconTheme.

static void fmIconDataDestroy(gpointer data) {
  QIcon* picon = reinterpret_cast<QIcon*>(data);
  delete picon;
}

IconTheme::IconTheme():
  fallbackIcon(QIcon::fromTheme("unknown")) {
  // NOTE: only one instance is allowed
  Q_ASSERT(theIconTheme == NULL);

  theIconTheme = this;
  fm_icon_set_user_data_destroy(reinterpret_cast<GDestroyNotify>(fmIconDataDestroy));
}

IconTheme::~IconTheme() {
}

void IconTheme::setThemeName(QString name) {
  QIcon::setThemeName(name);
  if(theIconTheme) {
    // set fallback icon
    theIconTheme->fallbackIcon = QIcon::fromTheme("unknown");
  }
}

QIcon IconTheme::convertFromGIcon(GIcon* gicon) {
  if(G_IS_THEMED_ICON(gicon)) {
    const gchar * const * names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
    const gchar * const * name;
    for(name = names; *name; ++name) {
      // qDebug("icon name=%s", *name);
      QString qname = *name;
      QIcon qicon = QIcon::fromTheme(qname);
      if(!qicon.isNull()) {
	return qicon;
      }
    }
  }
  else if(G_IS_FILE_ICON(gicon)) {
    GFile* file = g_file_icon_get_file(G_FILE_ICON(gicon));
    char* fpath = g_file_get_path(file);
    QString path = fpath;
    g_free(fpath);
    return QIcon(path);
  }
  return theIconTheme->fallbackIcon;
}


//static
QIcon IconTheme::icon(FmIcon* fmicon) {
  // check if we have a cached version
  QIcon* picon = reinterpret_cast<QIcon*>(fm_icon_get_user_data(fmicon));
  if(!picon) { // we don't have a cache yet
    picon = new QIcon(); // what a waste!
    *picon = convertFromGIcon(fmicon->gicon);
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
  return theIconTheme->fallbackIcon;
}

