/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

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
#include <iostream>

using namespace Fm;

struct PixEntry {
  int size;
  QPixmap pixmap;
};

static void fmIconDataDestroy(gpointer data) {
  GSList* pixs = (GSList*)data;
  GSList* l;
  for(l = pixs; l; l=l->next) {
      PixEntry* ent = (PixEntry*)l->data;
      delete ent;
  }
  g_slist_free(pixs);
}

IconTheme::IconTheme() {
  // FIXME: only one instance is allowed
  fm_icon_set_user_data_destroy(reinterpret_cast<GDestroyNotify>(fmIconDataDestroy));
}

IconTheme::~IconTheme() {
}

// Qt4 has QIcon::fromTheme() which loads an icon according to
// freedesktop.org icon theme spec.
// However, it does not handle GIcon, fallback icon names, and
// does no do cache. So let's integrate it with FmIcon.
QPixmap IconTheme::loadIcon(FmIcon* icon, int size) {
  GIcon* gicon = icon->gicon;
  GSList* pixs = (GSList*)fm_icon_get_user_data(icon);
  GSList* l;
  PixEntry* entry = NULL;
  if(G_IS_THEMED_ICON(gicon)) {
    for(l = pixs; l; l = l->next) {
      entry = (PixEntry*)l->data;
      if(entry->size == size)
	break;
    }
    if(!l) { // if not found, add a new entry
      // std::cout << "NOT FOUND" << std::endl;
      entry = new PixEntry;
      entry->size = size;
      pixs = g_slist_prepend(pixs, entry);
      fm_icon_set_user_data(icon, pixs);
      // load pixmap
      const gchar * const * names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
      const gchar * const * name;
      for(name = names; *name; ++name) {
	QString qname = *name;
	if(QIcon::hasThemeIcon(qname)) {
	  QIcon qicon = QIcon::fromTheme(qname);
	  entry->pixmap = qicon.pixmap(size, size);
	  break;
	}
      }
      if(*names == NULL) // not found
	entry->pixmap = QPixmap(); //FIXME: use fallback icon
    }
    else {
      // std::cout << "FOUND" << std::endl;
    }
    return entry->pixmap;
  }
  else if(G_IS_FILE_ICON(gicon)) {
  }
  return QPixmap(); // fallback
}
