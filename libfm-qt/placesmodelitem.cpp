/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "placesmodel.h"
#include "icontheme.h"
#include <gio/gio.h>

namespace Fm {

PlacesModelItem::PlacesModelItem():
  QStandardItem(),
  path_(NULL),
  icon_(NULL),
  fileInfo_(NULL) {
}

PlacesModelItem::PlacesModelItem(const char* iconName, QString title, FmPath* path):
  QStandardItem(title),
  path_(path ? fm_path_ref(path) : NULL),
  icon_(fm_icon_from_name(iconName)),
  fileInfo_(NULL) {
  if(icon_)
    QStandardItem::setIcon(IconTheme::icon(icon_));
}

PlacesModelItem::PlacesModelItem(FmIcon* icon, QString title, FmPath* path):
  QStandardItem(title),
  path_(path ? fm_path_ref(path) : NULL),
  icon_(icon ? fm_icon_ref(icon) : NULL),
  fileInfo_(NULL) {
  if(icon_)
    QStandardItem::setIcon(IconTheme::icon(icon));
}

PlacesModelItem::PlacesModelItem(QIcon icon, QString title, FmPath* path):
  QStandardItem(icon, title),
  path_(path ? fm_path_ref(path) : NULL),
  fileInfo_(NULL) {
}

PlacesModelItem::~PlacesModelItem() {
  if(path_)
    fm_path_unref(path_);
  if(fileInfo_)
    g_object_unref(fileInfo_);
  if(icon_)
    fm_icon_unref(icon_);
}

void PlacesModelItem::setPath(FmPath* path) {
  if(path_)
    fm_path_unref(path_);
  path_ = path ? fm_path_ref(path) : NULL;
}

void PlacesModelItem::setIcon(FmIcon* icon) {
  if(icon_)
    fm_icon_unref(icon_);
  if(icon) {
    icon_ = fm_icon_ref(icon);
    QStandardItem::setIcon(IconTheme::icon(icon_));
  }
  else {
    icon_ = NULL;
    QStandardItem::setIcon(QIcon());
  }
}

void PlacesModelItem::setIcon(GIcon* gicon) {
  FmIcon* icon = gicon ? fm_icon_from_gicon(gicon) : NULL;
  setIcon(icon);
  fm_icon_unref(icon);
}

void PlacesModelItem::updateIcon() {
  if(icon_)
    QStandardItem::setIcon(IconTheme::icon(icon_));
}

QVariant PlacesModelItem::data(int role) const {
  // we use a QPixmap from FmIcon cache rather than QIcon object for decoration role.
  return QStandardItem::data(role);
}

void PlacesModelItem::setFileInfo(FmFileInfo* fileInfo) {
  // FIXME: how can we correctly update icon?
  if(fileInfo_)
    fm_file_info_unref(fileInfo_);

  if(fileInfo) {
    fileInfo_ = fm_file_info_ref(fileInfo);
  }
  else
    fileInfo_ = NULL;
}

PlacesModelBookmarkItem::PlacesModelBookmarkItem(FmBookmarkItem* bm_item):
  PlacesModelItem(QIcon::fromTheme("folder"), QString::fromUtf8(bm_item->name), bm_item->path),
  bookmarkItem_(fm_bookmark_item_ref(bm_item)) {
  setEditable(true);
}

PlacesModelVolumeItem::PlacesModelVolumeItem(GVolume* volume):
  PlacesModelItem(),
  volume_(reinterpret_cast<GVolume*>(g_object_ref(volume))) {
  update();
  setEditable(false);
}

void PlacesModelVolumeItem::update() {
  // set title
  setText(QString::fromUtf8(g_volume_get_name(volume_)));
  
  // set icon
  GIcon* gicon = g_volume_get_icon(volume_);
  setIcon(gicon);
  g_object_unref(gicon);

  // set dir path
  GMount* mount = g_volume_get_mount(volume_);
  if(mount) {
    GFile* mount_root = g_mount_get_root(mount);
    FmPath* mount_path = fm_path_new_for_gfile(mount_root);
    setPath(mount_path);
    fm_path_unref(mount_path);
    g_object_unref(mount_root);
    g_object_unref(mount);
  }
  else {
    setPath(NULL);
  }
}


bool PlacesModelVolumeItem::isMounted() {
  GMount* mount = g_volume_get_mount(volume_);
  if(mount)
    g_object_unref(mount);
  return mount != NULL ? true : false;
}


PlacesModelMountItem::PlacesModelMountItem(GMount* mount):
  PlacesModelItem(),
  mount_(reinterpret_cast<GMount*>(mount)) {
  update();
  setEditable(false);
}

void PlacesModelMountItem::update() {
  // set title
  setText(QString::fromUtf8(g_mount_get_name(mount_)));
  
  // set path
  GFile* mount_root = g_mount_get_root(mount_);
  FmPath* mount_path = fm_path_new_for_gfile(mount_root);
  setPath(mount_path);
  fm_path_unref(mount_path);
  g_object_unref(mount_root);
  
  // set icon
  GIcon* gicon = g_mount_get_icon(mount_);
  setIcon(gicon);
  g_object_unref(gicon);
}

}
