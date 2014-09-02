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
#include <QDebug>
#include <QMimeData>
#include <QTimer>
#include "utilities.h"
#include "placesmodelitem.h"

using namespace Fm;

PlacesModel::PlacesModel(QObject* parent):
  QStandardItemModel(parent),
  showApplications_(true),
  showDesktop_(true),
  ejectIcon_(QIcon::fromTheme("media-eject")) {

  setColumnCount(2);

  PlacesModelItem* item;
  placesRoot = new QStandardItem(tr("Places"));
  placesRoot->setEditable(false);
  placesRoot->setSelectable(false);
  placesRoot->setColumnCount(2);
  appendRow(placesRoot);

  homeItem = new PlacesModelItem("user-home", g_get_user_name(), fm_path_get_home());
  homeItem->setEditable(false);
  placesRoot->appendRow(homeItem);

  desktopItem = new PlacesModelItem("user-desktop", tr("Desktop"), fm_path_get_desktop());
  desktopItem->setEditable(false);
  placesRoot->appendRow(desktopItem);

  createTrashItem();

  FmPath* path;
  if(isUriSchemeSupported("computer")) {
    path = fm_path_new_for_uri("computer:///");
    computerItem = new PlacesModelItem("computer", tr("Computer"), path);
    fm_path_unref(path);
    computerItem->setEditable(false);
    placesRoot->appendRow(computerItem);
  }
  else
    computerItem = NULL;

  const char* applicaion_icon_names[] = {"system-software-install", "applications-accessories", "application-x-executable"};
  // NOTE: g_themed_icon_new_from_names() accepts char**, but actually const char** is OK.
  GIcon* gicon = g_themed_icon_new_from_names((char**)applicaion_icon_names, G_N_ELEMENTS(applicaion_icon_names));
  FmIcon* fmicon = fm_icon_from_gicon(gicon);
  g_object_unref(gicon);
  applicationsItem = new PlacesModelItem(fmicon, tr("Applications"), fm_path_get_apps_menu());
  fm_icon_unref(fmicon);
  applicationsItem->setEditable(false);
  placesRoot->appendRow(applicationsItem);

  if(isUriSchemeSupported("network")) {
    const char* network_icon_names[] = {"network", "folder-network", "folder"};
    // NOTE: g_themed_icon_new_from_names() accepts char**, but actually const char** is OK.
    gicon = g_themed_icon_new_from_names((char**)network_icon_names, G_N_ELEMENTS(network_icon_names));
    fmicon = fm_icon_from_gicon(gicon);
    g_object_unref(gicon);
    path = fm_path_new_for_uri("network:///");
    networkItem = new PlacesModelItem(fmicon, tr("Network"), path);
    fm_icon_unref(fmicon);
    fm_path_unref(path);
    networkItem->setEditable(false);
    placesRoot->appendRow(networkItem);
  }
  else
    networkItem = NULL;

  devicesRoot = new QStandardItem(tr("Devices"));
  devicesRoot->setEditable(false);
  devicesRoot->setSelectable(false);
  devicesRoot->setColumnCount(2);
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

    // add volumes to side-pane
    GList* vols = g_volume_monitor_get_volumes(volumeMonitor);
    GList* l;
    for(l = vols; l; l = l->next) {
        GVolume* volume = G_VOLUME(l->data);
        onVolumeAdded(volumeMonitor, volume, this);
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
        item = new PlacesModelMountItem(mount);
        devicesRoot->appendRow(item);
        }
        g_object_unref(mount);
    }
    g_list_free(vols);
  }

  // bookmarks
  bookmarksRoot = new QStandardItem(tr("Bookmarks"));
  bookmarksRoot->setEditable(false);
  bookmarksRoot->setSelectable(false);
  bookmarksRoot->setColumnCount(2);
  appendRow(bookmarksRoot);

  bookmarks = fm_bookmarks_dup();
  loadBookmarks();
  g_signal_connect(bookmarks, "changed", G_CALLBACK(onBookmarksChanged), this);

  // update some icons when the icon theme is changed
  connect(IconTheme::instance(), SIGNAL(changed()), SLOT(updateIcons()));
}

void PlacesModel::loadBookmarks() {
  GList* allBookmarks = fm_bookmarks_get_all(bookmarks);
  for(GList* l = allBookmarks; l; l = l->next) {
    FmBookmarkItem* bm_item = (FmBookmarkItem*)l->data;
    PlacesModelBookmarkItem* item = new PlacesModelBookmarkItem(bm_item);
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
  if(trashMonitor_) {
    g_signal_handlers_disconnect_by_func(trashMonitor_, (gpointer)G_CALLBACK(onTrashChanged), this);
    g_object_unref(trashMonitor_);
  }
}

// static
void PlacesModel::onTrashChanged(GFileMonitor* monitor, GFile* gf, GFile* other, GFileMonitorEvent evt, PlacesModel* pThis) {
  QTimer::singleShot(0, pThis, SLOT(updateTrash()));
}

void PlacesModel::updateTrash() {
  if(trashItem_) {
    GFile* gf = fm_file_new_for_uri("trash:///");
    GFileInfo* inf = g_file_query_info(gf, G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    g_object_unref(gf);
    if(inf) {
      guint32 n = g_file_info_get_attribute_uint32(inf, G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT);
      g_object_unref(inf);
      const char* icon_name = n > 0 ? "user-trash-full" : "user-trash";
      FmIcon* icon = fm_icon_from_name(icon_name);
      trashItem_->setIcon(icon);
      fm_icon_unref(icon);
    }
  }
}

void PlacesModel::createTrashItem() {
  GFile* gf;
  gf = fm_file_new_for_uri("trash:///");
  // check if trash is supported by the current vfs
  // if gvfs is not installed, this can be unavailable.
  if(!g_file_query_exists(gf, NULL)) {
    g_object_unref(gf);
    trashItem_ = NULL;
    trashMonitor_ = NULL;
    return;
  }
  trashItem_ = new PlacesModelItem("user-trash", tr("Trash"), fm_path_get_trash());
  trashItem_->setEditable(false);

  trashMonitor_ = fm_monitor_directory(gf, NULL);
  if(trashMonitor_)
    g_signal_connect(trashMonitor_, "changed", G_CALLBACK(onTrashChanged), this);
  g_object_unref(gf);

  placesRoot->insertRow(desktopItem->row() + 1, trashItem_);
  QTimer::singleShot(0, this, SLOT(updateTrash()));
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
  if(show) {
    if(!trashItem_)
      createTrashItem();
  }
  else {
    if(trashItem_) {
      if(trashMonitor_) {
        g_signal_handlers_disconnect_by_func(trashMonitor_, (gpointer)G_CALLBACK(onTrashChanged), this);
        g_object_unref(trashMonitor_);
        trashMonitor_ = NULL;
      }
      placesRoot->removeRow(trashItem_->row()); // delete trashItem_;
      trashItem_ = NULL;
    }
  }
}

PlacesModelItem* PlacesModel::itemFromPath(FmPath* path) {
  PlacesModelItem* item = itemFromPath(placesRoot, path);
  if(!item)
    item = itemFromPath(devicesRoot, path);
  if(!item)
    item = itemFromPath(bookmarksRoot, path);
  return item;
}

PlacesModelItem* PlacesModel::itemFromPath(QStandardItem* rootItem, FmPath* path) {
  int rowCount = rootItem->rowCount();
  for(int i = 0; i < rowCount; ++i) {
    PlacesModelItem* item = static_cast<PlacesModelItem*>(rootItem->child(i, 0));
    if(fm_path_equal(item->path(), path))
      return item;
  }
  return NULL;
}

PlacesModelVolumeItem* PlacesModel::itemFromVolume(GVolume* volume) {
  int rowCount = devicesRoot->rowCount();
  for(int i = 0; i < rowCount; ++i) {
    PlacesModelItem* item = static_cast<PlacesModelItem*>(devicesRoot->child(i, 0));
    if(item->type() == PlacesModelItem::Volume) {
      PlacesModelVolumeItem* volumeItem = static_cast<PlacesModelVolumeItem*>(item);
      if(volumeItem->volume() == volume)
        return volumeItem;
    }
  }
  return NULL;
}

PlacesModelMountItem* PlacesModel::itemFromMount(GMount* mount) {
  int rowCount = devicesRoot->rowCount();
  for(int i = 0; i < rowCount; ++i) {
    PlacesModelItem* item = static_cast<PlacesModelItem*>(devicesRoot->child(i, 0));
    if(item->type() == PlacesModelItem::Mount) {
      PlacesModelMountItem* mountItem = static_cast<PlacesModelMountItem*>(item);
      if(mountItem->mount() == mount)
        return mountItem;
    }
  }
  return NULL;
}

PlacesModelBookmarkItem* PlacesModel::itemFromBookmark(FmBookmarkItem* bkitem) {
  int rowCount = bookmarksRoot->rowCount();
  for(int i = 0; i < rowCount; ++i) {
    PlacesModelBookmarkItem* item = static_cast<PlacesModelBookmarkItem*>(bookmarksRoot->child(i, 0));
    if(item->bookmark() == bkitem)
      return item;
  }
  return NULL;
}

void PlacesModel::onMountAdded(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis) {
  GVolume* vol = g_mount_get_volume(mount);
  if(vol) { // mount-added is also emitted when a volume is newly mounted.
    PlacesModelVolumeItem* item = pThis->itemFromVolume(vol);
    if(item && !item->path()) {
      // update the mounted volume and show a button for eject.
      GFile* gf = g_mount_get_root(mount);
      FmPath* path = fm_path_new_for_gfile(gf);
      g_object_unref(gf);
      item->setPath(path);
      if(path)
        fm_path_unref(path);
      // update the mount indicator (eject button)
      QStandardItem* ejectBtn = item->parent()->child(item->row(), 1);
      Q_ASSERT(ejectBtn);
      ejectBtn->setIcon(pThis->ejectIcon_);
    }
    g_object_unref(vol);
  }
  else { // network mounts and others
    PlacesModelMountItem* item = pThis->itemFromMount(mount);
    /* for some unknown reasons, sometimes we get repeated mount-added
     * signals and added a device more than one. So, make a sanity check here. */
    if(!item) {
      item = new PlacesModelMountItem(mount);
      QStandardItem* eject_btn = new QStandardItem(pThis->ejectIcon_, "");
      pThis->devicesRoot->appendRow(QList<QStandardItem*>() << item << eject_btn);
    }
  }
}

void PlacesModel::onMountChanged(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis) {
  PlacesModelMountItem* item = pThis->itemFromMount(mount);
  if(item)
    item->update();
}

void PlacesModel::onMountRemoved(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis) {
  GVolume* vol = g_mount_get_volume(mount);
  qDebug() << "volume umounted: " << vol;
  if(vol) {
    // a volume is unmounted
    g_object_unref(vol);
  }
  else { // network mounts and others
    PlacesModelMountItem* item = pThis->itemFromMount(mount);
    if(item) {
      pThis->devicesRoot->removeRow(item->row());
    }
  }
}

void PlacesModel::onVolumeAdded(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis) {
  // for some unknown reasons, sometimes we get repeated volume-added
  // signals and added a device more than one. So, make a sanity check here.
  PlacesModelVolumeItem* volumeItem = pThis->itemFromVolume(volume);
  if(!volumeItem) {
    volumeItem = new PlacesModelVolumeItem(volume);
    QStandardItem* ejectBtn = new QStandardItem();
    if(volumeItem->isMounted())
      ejectBtn->setIcon(pThis->ejectIcon_);
    pThis->devicesRoot->appendRow(QList<QStandardItem*>() << volumeItem << ejectBtn);
  }
}

void PlacesModel::onVolumeChanged(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis) {
  PlacesModelVolumeItem* item = pThis->itemFromVolume(volume);
  if(item) {
    item->update();
    if(!item->isMounted()) { // the volume is unmounted, remove the eject button if needed
      // remove the eject button for the volume (at column 1 of the same row)
      QStandardItem* ejectBtn = item->parent()->child(item->row(), 1);
      Q_ASSERT(ejectBtn);
      ejectBtn->setIcon(QIcon());
    }
  }
}

void PlacesModel::onVolumeRemoved(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis) {
  PlacesModelVolumeItem* item = pThis->itemFromVolume(volume);
  if(item) {
    pThis->devicesRoot->removeRow(item->row());
  }
}

void PlacesModel::onBookmarksChanged(FmBookmarks* bookmarks, PlacesModel* pThis) {
  // remove all items
  pThis->bookmarksRoot->removeRows(0, pThis->bookmarksRoot->rowCount());
  pThis->loadBookmarks();
}

void PlacesModel::updateIcons() {
  // the icon theme is changed and we need to update the icons
  PlacesModelItem* item;
  int row;
  int n = placesRoot->rowCount();
  for(row = 0; row < n; ++row) {
    item = static_cast<PlacesModelItem*>(placesRoot->child(row));
    item->updateIcon();
  }
  n = devicesRoot->rowCount();
  for(row = 0; row < n; ++row) {
    item = static_cast<PlacesModelItem*>(devicesRoot->child(row));
    item->updateIcon();
  }
}

Qt::ItemFlags PlacesModel::flags(const QModelIndex& index) const {
  if(index.column() == 1) // make 2nd column of every row selectable.
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  if(!index.parent().isValid()) { // root items
    if(index.row() == 2) // bookmarks root
      return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    else
      return Qt::ItemIsEnabled;
  }
  PlacesModelItem* placesItem = static_cast<PlacesModelItem*>(itemFromIndex(index));
  if(placesItem) {
    if(placesItem->type() == PlacesModelItem::Bookmark)
      return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
  }
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
  // return QStandardItemModel::flags(index);
}

bool PlacesModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
  QStandardItem* item = itemFromIndex(parent);
  if(data->hasFormat("application/x-bookmark-row")) { // the data being dopped is a bookmark row
    // decode it and do bookmark reordering
    QByteArray buf = data->data("application/x-bookmark-row");
    QDataStream stream(&buf, QIODevice::ReadOnly);
    int oldPos = -1;
    char* pathStr = NULL;
    stream >> oldPos >> pathStr;
    // find the source bookmark item being dragged
    GList* allBookmarks = fm_bookmarks_get_all(bookmarks);
    FmBookmarkItem* draggedItem = static_cast<FmBookmarkItem*>(g_list_nth_data(allBookmarks, oldPos));
    // If we cannot find the dragged bookmark item at position <oldRow>, or we find an item,
    // but the path of the item is not the same as what we expected, than it's the wrong item.
    // This means that the bookmarks are changed during our dnd processing, which is an extremely rare case.
    if(!draggedItem || !fm_path_equal_str(draggedItem->path, pathStr, -1)) {
      delete []pathStr;
      return false;
    }
    delete []pathStr;

    int newPos = -1;
    if(row == -1 && column == -1) { // drop on an item
      // we only allow dropping on an bookmark item
      if(item && item->parent() == bookmarksRoot)
        newPos = parent.row();
    }
    else { // drop on a position between items
      if(item == bookmarksRoot) // we only allow dropping on a bookmark item
        newPos = row;
    }
    if(newPos != -1 && newPos != oldPos) // reorder the bookmark item
      fm_bookmarks_reorder(bookmarks, draggedItem, newPos);
  }
  else if(data->hasUrls()) { // files uris are dropped
    if(row == -1 && column == -1) { // drop uris on an item
      if(item && item->parent()) { // need to be a child item
        PlacesModelItem* placesItem = static_cast<PlacesModelItem*>(item);
        if(placesItem->path()) {
          qDebug() << "dropped dest:" << placesItem->text();
          // TODO: copy or move the dragged files to the dir pointed by the item.
          qDebug() << "drop on" << item->text();
        }
      }
    }
    else { // drop uris on a position between items
      if(item == bookmarksRoot) { // we only allow dropping on blank row of bookmarks section
        FmPathList* paths = pathListFromQUrls(data->urls());
        for(GList* l = fm_path_list_peek_head_link(paths); l; l = l->next) {
          FmPath* path = FM_PATH(l->data);
          GFile* gf = fm_path_to_gfile(path);
          // FIXME: this is a blocking call
          if(g_file_query_file_type(gf, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                    NULL) == G_FILE_TYPE_DIRECTORY) {
            char* disp_name = fm_path_display_basename(path);
            fm_bookmarks_insert(bookmarks, path, disp_name, row);
            g_free(disp_name);
          }
          g_object_unref(gf);
          return true;
        }
      }
    }
  }
  return false;
}

// we only support dragging bookmark items and use our own
// custom pseudo-mime-type: application/x-bookmark-row
QMimeData* PlacesModel::mimeData(const QModelIndexList& indexes) const {
  if(!indexes.isEmpty()) {
    // we only allow dragging one bookmark item at a time, so handle the first index only.
    QModelIndex index = indexes.first();
    QStandardItem* item = itemFromIndex(index);
    // ensure that it's really a bookmark item
    if(item && item->parent() == bookmarksRoot) {
      PlacesModelBookmarkItem* bookmarkItem = static_cast<PlacesModelBookmarkItem*>(item);
      QMimeData* mime = new QMimeData();
      QByteArray data;
      QDataStream stream(&data, QIODevice::WriteOnly);
      // There is no safe and cross-process way to store a reference of a row.
      // Let's store the pos, name, and path of the bookmark item instead.
      char* pathStr = fm_path_to_str(bookmarkItem->path());
      stream << index.row() << pathStr;
      g_free(pathStr);
      mime->setData("application/x-bookmark-row", data);
      return mime;
    }
  }
  return NULL;
}

QStringList PlacesModel::mimeTypes() const {
  return QStringList() << "application/x-bookmark-row" << "text/uri-list";
}

Qt::DropActions PlacesModel::supportedDropActions() const {
  return QStandardItemModel::supportedDropActions();
}

