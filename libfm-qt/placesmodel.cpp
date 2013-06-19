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

using namespace Fm;

PlacesModel::Item::Item(FmPath* path):
  QStandardItem(),
  fileInfo_(NULL) {
  path_ = path ? fm_path_ref(path) : NULL;
}

PlacesModel::Item::Item(const char* iconName, QString title, FmPath* path):
  QStandardItem(title),
  fileInfo_(NULL) {
  path_ = path ? fm_path_ref(path) : NULL;
  FmIcon* icon = fm_icon_from_name(iconName);
  setIcon(IconTheme::icon(icon));
  fm_icon_unref(icon);
}

PlacesModel::Item::Item(FmIcon* icon, QString title, FmPath* path):
  QStandardItem(title),
  fileInfo_(NULL) {
  path_ = path ? fm_path_ref(path) : NULL;
  setIcon(IconTheme::icon(icon));
}

PlacesModel::Item::Item(GIcon* gicon, QString title, FmPath* path):
  QStandardItem(title),
  fileInfo_(NULL) {
  path_ = path ? fm_path_ref(path) : NULL;
  FmIcon* icon = fm_icon_from_gicon(gicon);
  setIcon(IconTheme::icon(gicon));
  fm_icon_unref(icon);
}

PlacesModel::Item::Item(QIcon icon, QString title, FmPath* path):
  QStandardItem(icon, title),
  fileInfo_(NULL) {
  path_ = path ? fm_path_ref(path) : NULL;
}

PlacesModel::Item::Item(QString title, FmPath* path):
  QStandardItem(title),
  fileInfo_(NULL) {
  path_ = path ? fm_path_ref(path) : NULL;
}

PlacesModel::Item::~Item() {
  if(path_)
    fm_path_unref(path_);
  if(fileInfo_)
    g_object_unref(fileInfo_);
}

void PlacesModel::Item::setPath(FmPath* path) {
  if(path_)
    fm_path_unref(path_);
  path_ = path ? fm_path_ref(path) : NULL;
}


QVariant PlacesModel::Item::data(int role) const {
  // we use a QPixmap from FmIcon cache rather than QIcon object for decoration role.
  return QStandardItem::data(role);
}

void PlacesModel::Item::setFileInfo(FmFileInfo* fileInfo) {
  // FIXME: how can we correctly update icon?
  if(fileInfo_)
    fm_file_info_unref(fileInfo_);

  if(fileInfo) {
    fileInfo_ = fm_file_info_ref(fileInfo);
  }
  else
    fileInfo_ = NULL;
}


PlacesModel::PlacesModel(QObject* parent):
  QStandardItemModel(parent),
  showTrash_(true),
  showApplications_(true),
  showDesktop_(true) {
  Item *item;
  placesRoot = new QStandardItem(tr("Places"));
  placesRoot->setEditable(false);
  placesRoot->setSelectable(false);
  appendRow(placesRoot);

  homeItem = new Item("user-home", g_get_user_name(), fm_path_get_home());
  homeItem->setEditable(false);
  placesRoot->appendRow(homeItem);

  desktopItem = new Item("user-desktop", tr("Desktop"), fm_path_get_desktop());
  desktopItem->setEditable(false);
  placesRoot->appendRow(desktopItem);

  trashItem = new Item("user-trash", tr("Trash"), fm_path_get_trash());
  trashItem->setEditable(false);
  placesRoot->appendRow(trashItem);

  FmPath* path = fm_path_new_for_uri("computer:///");
  computerItem = new Item("computer", tr("Computer"), path);
  fm_path_unref(path);
  computerItem->setEditable(false);
  placesRoot->appendRow(computerItem);

  applicationsItem = new Item(
    QIcon::fromTheme("system-software-install", QIcon::fromTheme("applications-accessories")),
                              tr("Applications"), fm_path_get_apps_menu());
  applicationsItem->setEditable(false);
  placesRoot->appendRow(applicationsItem);

  path = fm_path_new_for_uri("network:///");
  networkItem = new Item(QIcon::fromTheme("network", QIcon::fromTheme("folder-network")), tr("Network"), path);
  fm_path_unref(path);
  networkItem->setEditable(false);
  placesRoot->appendRow(networkItem);

  devicesRoot = new QStandardItem(tr("Devices"));
  devicesRoot->setEditable(false);
  devicesRoot->setSelectable(false);
  appendRow(devicesRoot);

  // volumes
  volumeMonitor = g_volume_monitor_get();
  if(volumeMonitor) {
    g_signal_connect(volumeMonitor, "volume-added", G_CALLBACK(onVolumeAdded), this);
    g_signal_connect(volumeMonitor, "volume-removed", G_CALLBACK(onVolumeRemoved), this);
    g_signal_connect(volumeMonitor, "volume-changed", G_CALLBACK(onVolumeChanged), this);
    g_signal_connect(volumeMonitor, "mount-added", G_CALLBACK(onMountAdded), this);
    g_signal_connect(volumeMonitor, "mount-changed", G_CALLBACK(onMountChanged), this);
    g_signal_connect(volumeMonitor, "mount-removed", G_CALLBACK(onMountRemoved), this);
  }

  // add volumes to side-pane
  GList* vols = g_volume_monitor_get_volumes(volumeMonitor);
  GList* l;
  for(l = vols; l; l = l->next) {
    GVolume* volume = G_VOLUME(l->data);
    item = new VolumeItem(volume);
    devicesRoot->appendRow(item);
    g_object_unref(volume);
  }
  g_list_free(vols);

  /* add mounts to side-pane */
  vols = g_volume_monitor_get_mounts(volumeMonitor);
  for(l = vols; l; l = l->next) {
    GMount* mount = G_MOUNT(l->data);
    GVolume* volume = g_mount_get_volume(mount);
    if(volume)
      g_object_unref(volume);
    else { /* network mounts or others */
      item = new MountItem(mount);
      devicesRoot->appendRow(item);
    }
    g_object_unref(mount);
  }
  g_list_free(vols);

  // bookmarks
  bookmarksRoot = new QStandardItem(tr("Bookmarks"));
  bookmarksRoot->setEditable(false);
  bookmarksRoot->setSelectable(false);
  appendRow(bookmarksRoot);
  
  bookmarks = fm_bookmarks_dup();
  loadBookmarks();
  g_signal_connect(bookmarks, "changed", G_CALLBACK(onBookmarksChanged), this);
}

void PlacesModel::loadBookmarks() {
  GList* allBookmarks = fm_bookmarks_get_all(bookmarks);
  for(GList* l = allBookmarks; l; l = l->next) {
    FmBookmarkItem* bm_item = (FmBookmarkItem*)l->data;
    BookmarkItem* item = new BookmarkItem(bm_item);
    bookmarksRoot->appendRow(item);
  }
  g_list_free_full(allBookmarks, (GDestroyNotify)fm_bookmark_item_unref);
}

PlacesModel::~PlacesModel() {
  if(bookmarks) {
    g_signal_handlers_disconnect_by_func(bookmarks, (gpointer)onBookmarksChanged, this);
    g_object_unref(bookmarks);
  }
  if(volumeMonitor) {
    g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onVolumeAdded), this);
    g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onVolumeRemoved), this);
    g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onVolumeChanged), this);
    g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onMountAdded), this);
    g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onMountChanged), this);
    g_signal_handlers_disconnect_by_func(volumeMonitor, (gpointer)G_CALLBACK(onMountRemoved), this);
    g_object_unref(volumeMonitor);
  }
}

PlacesModel::BookmarkItem::BookmarkItem(FmBookmarkItem* bm_item):
  Item("folder", QString::fromUtf8(bm_item->name), bm_item->path),
  bookmarkItem_(fm_bookmark_item_ref(bm_item)) {
  setEditable(true);
}

PlacesModel::VolumeItem::VolumeItem(GVolume* volume):
  Item(NULL),
  volume_(reinterpret_cast<GVolume*>(g_object_ref(volume))) {
  update();
  setEditable(false);
}

void PlacesModel::VolumeItem::update() {
  // set title
  setText(QString::fromUtf8(g_volume_get_name(volume_)));
  
  // set icon
  GIcon* gicon = g_volume_get_icon(volume_);
  setIcon(IconTheme::icon(gicon));
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


bool PlacesModel::VolumeItem::isMounted() {
  GMount* mount = g_volume_get_mount(volume_);
  if(mount)
    g_object_unref(mount);
  return mount != NULL ? true : false;
}


PlacesModel::MountItem::MountItem(GMount* mount):
  Item(NULL),
  mount_(reinterpret_cast<GMount*>(mount)) {
  update();
  setEditable(false);
}

void PlacesModel::MountItem::update() {
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
  setIcon(IconTheme::icon(gicon));
  g_object_unref(gicon);
}


void PlacesModel::setShowApplications(bool show) {
  if(showApplications_ != show) {
    showApplications_ = show;
  }
}

void PlacesModel::setShowDesktop(bool show) {
  if(showDesktop_ != show) {
    showDesktop_ = show;
  }
}

void PlacesModel::setShowTrash(bool show) {
  if(showTrash_ != show) {
    showTrash_ = show;
  }
}

PlacesModel::Item* PlacesModel::itemFromPath(FmPath* path) {
  Item* item = itemFromPath(placesRoot, path);
  if(!item)
    item = itemFromPath(devicesRoot, path);
  if(!item)
    item = itemFromPath(bookmarksRoot, path);
  return item;
}

PlacesModel::Item* PlacesModel::itemFromPath(QStandardItem* rootItem, FmPath* path) {
  int rowCount = rootItem->rowCount();
  for(int i = 0; i < rowCount; ++i) {
    Item* item = static_cast<Item*>(rootItem->child(i, 0));
    if(fm_path_equal(item->path(), path))
      return item;
  }
  return NULL;
}

PlacesModel::VolumeItem* PlacesModel::itemFromVolume(GVolume* volume) {
  int rowCount = devicesRoot->rowCount();
  for(int i = 0; i < rowCount; ++i) {
    Item* item = static_cast<Item*>(devicesRoot->child(i, 0));
    if(item->type() == ItemTypeVolume) {
      VolumeItem* volumeItem = static_cast<VolumeItem*>(item);
      if(volumeItem->volume() == volume)
        return volumeItem;
    }
  }
  return NULL;
}

PlacesModel::MountItem* PlacesModel::itemFromMount(GMount* mount) {
  int rowCount = devicesRoot->rowCount();
  for(int i = 0; i < rowCount; ++i) {
    Item* item = static_cast<Item*>(devicesRoot->child(i, 0));
    if(item->type() == ItemTypeMount) {
      MountItem* mountItem = static_cast<MountItem*>(item);
      if(mountItem->mount() == mount)
        return mountItem;
    }
  }
  return NULL;
}

PlacesModel::BookmarkItem* PlacesModel::itemFromBookmark(FmBookmarkItem* bkitem) {
  int rowCount = bookmarksRoot->rowCount();
  for(int i = 0; i < rowCount; ++i) {
    BookmarkItem* item = static_cast<BookmarkItem*>(bookmarksRoot->child(i, 0));
    if(item->bookmark() == bkitem)
      return item;
  }
  return NULL;
}

void PlacesModel::onMountAdded(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis) {
  GVolume* vol = g_mount_get_volume(mount);
  if(vol) { // mount-added is also emitted when a volume is newly mounted.
    VolumeItem* item = pThis->itemFromVolume(vol);
    if(item && !item->path()) {
      // update the mounted volume and show a button for eject.
      GFile* gf = g_mount_get_root(mount);
      FmPath* path = fm_path_new_for_gfile(gf);
      g_object_unref(gf);
      item->setPath(path);
      if(path)
        fm_path_unref(path);
      // TODO: inform the view to update mount indicator
    }
    g_object_unref(vol);
  }
  else { // network mounts and others
    MountItem* item = pThis->itemFromMount(mount);
    /* for some unknown reasons, sometimes we get repeated mount-added 
     * signals and added a device more than one. So, make a sanity check here. */
    if(!item) {
      item = new MountItem(mount);
      pThis->devicesRoot->appendRow(item);
    }
  }
}

void PlacesModel::onMountChanged(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis) {
  MountItem* item = pThis->itemFromMount(mount);
  if(item)
    item->update();
}

void PlacesModel::onMountRemoved(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis) {
  GVolume* vol = g_mount_get_volume(mount);
  if(vol) // we handle volumes in volume-removed handler
    g_object_unref(vol);
  else { // network mounts and others
    MountItem* item = pThis->itemFromMount(mount);
    if(item) {
      pThis->devicesRoot->removeRow(item->row());
    }
  }
}

void PlacesModel::onVolumeAdded(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis) {
  // for some unknown reasons, sometimes we get repeated volume-added
  // signals and added a device more than one. So, make a sanity check here.
  VolumeItem* item = pThis->itemFromVolume(volume);
  if(!item) {
    item = new VolumeItem(volume);
    pThis->devicesRoot->appendRow(item);
  }
}

void PlacesModel::onVolumeChanged(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis) {
  VolumeItem* item = pThis->itemFromVolume(volume);
  if(item)
    item->update();
}

void PlacesModel::onVolumeRemoved(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis) {
  VolumeItem* item = pThis->itemFromVolume(volume);
  if(item) {
    pThis->devicesRoot->removeRow(item->row());
  }
}

void PlacesModel::onBookmarksChanged(FmBookmarks* bookmarks, PlacesModel* pThis) {
  // remove all items
  pThis->bookmarksRoot->removeRows(0, pThis->bookmarksRoot->rowCount());
  pThis->loadBookmarks();
}


/*

bool PlacesModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
  return QAbstractListModel::dropMimeData(data, action, row, column, parent);
}

QMimeData* PlacesModel::mimeData(const QModelIndexList& indexes) const {
  return QAbstractQStandardItemModel::mimeData(indexes);
}

Qt::DropActions PlacesModel::supportedDropActions() const {
  return QAbstractQStandardItemModel::supportedDropActions();
}
*/
