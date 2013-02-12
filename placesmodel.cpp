/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

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

PlacesModel::Item::Item():
  QStandardItem(),
  icon_(NULL),
  fileInfo_(NULL) {
}

PlacesModel::Item::Item(const char* iconName, QString title):
  QStandardItem(title),
  icon_(NULL),
  fileInfo_(NULL) {
  FmIcon* icon = fm_icon_from_name(iconName);
  setIcon(icon);
  fm_icon_unref(icon);
}

PlacesModel::Item::Item(FmIcon* icon, QString title):
  QStandardItem(title),
  icon_(NULL),
  fileInfo_(NULL) {
  setIcon(icon);
}

PlacesModel::Item::Item(GIcon* gicon, QString title):
  QStandardItem(title),
  icon_(NULL),
  fileInfo_(NULL) {
  FmIcon* icon = fm_icon_from_gicon(gicon);
  setIcon(icon);
  fm_icon_unref(icon);
}

PlacesModel::Item::Item(QIcon icon, QString title):
  QStandardItem(icon, title),
  icon_(NULL),
  fileInfo_(NULL) {
}

PlacesModel::Item::~Item() {
  if(icon_)
    fm_icon_unref(icon_);
  if(fileInfo_)
    g_object_unref(fileInfo_);
}

void PlacesModel::Item::setIcon(FmIcon* icon) {
  if(icon_)
    fm_icon_unref(icon_);
  if(icon) {
    icon_ = fm_icon_ref(icon);
  }
  else
    icon_ = NULL;
}

QVariant PlacesModel::Item::data(int role) const {
  // we use a QPixmap from FmIcon cache rather than QIcon object for decoration role.
  if(role == Qt::DecorationRole) {
    if(icon_) {
      QPixmap pix = IconTheme::loadIcon(icon_, fm_config->small_icon_size);
      return QVariant(pix);
    }
  }
  return QStandardItem::data(role);
}


PlacesModel::PlacesModel(QObject* parent) : QStandardItemModel(parent) {
  QStandardItem* rootItem;
  Item *item;
  rootItem = new QStandardItem("Places");
  rootItem->setEditable(false);
  rootItem->setSelectable(false);
  appendRow(rootItem);
  
  item = new Item("user-home", g_get_user_name());
  rootItem->appendRow(item);

  item = new Item("user-desktop", tr("Desktop"));
  rootItem->appendRow(item);

  item = new Item("user-trash", tr("Trash"));
  rootItem->appendRow(item);

  item = new Item("computer", tr("Computer"));
  rootItem->appendRow(item);

  item = new Item("system-software-install", tr("Applications"));
  rootItem->appendRow(item);

  item = new Item("network", tr("Network"));
  rootItem->appendRow(item);

  rootItem = new QStandardItem("Devices");
  rootItem->setEditable(false);
  rootItem->setSelectable(false);
  appendRow(rootItem);

  // volumes
  volumeMonitor = g_volume_monitor_get();
  if(volumeMonitor) {
    /*
    g_signal_connect(volumeMonitor, "volume-added", G_CALLBACK(on_volume_added), self);
    g_signal_connect(volumeMonitor, "volume-removed", G_CALLBACK(on_volume_removed), self);
    g_signal_connect(volumeMonitor, "volume-changed", G_CALLBACK(on_volume_changed), self);
    g_signal_connect(volumeMonitor, "mount-added", G_CALLBACK(on_mount_added), self);
    g_signal_connect(volumeMonitor, "mount-changed", G_CALLBACK(on_mount_changed), self);
    g_signal_connect(volumeMonitor, "mount-removed", G_CALLBACK(on_mount_removed), self);
    */
  }

  // add volumes to side-pane
  GList* vols = g_volume_monitor_get_volumes(volumeMonitor);
  GList* l;
  for(l = vols; l; l = l->next) {
    GVolume* volume = G_VOLUME(l->data);
    // add_volume_or_mount(self, G_OBJECT(vol), job);
    GIcon* gicon = g_volume_get_icon(volume);
    item = new Item(gicon, QString::fromUtf8(g_volume_get_name(volume)));
    rootItem->appendRow(item);
    g_object_unref(gicon);
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
      GIcon* gicon = g_mount_get_icon(mount);
      item = new Item(gicon, QString::fromUtf8(g_mount_get_name(mount)));
      rootItem->appendRow(item);
      g_object_unref(gicon);
      g_object_unref(mount);
    }
    g_object_unref(mount);
  }
  g_list_free(vols);

  // bookmarks
  rootItem = new QStandardItem("Bookmarks");
  rootItem->setEditable(false);
  rootItem->setSelectable(false);
  appendRow(rootItem);
  
  bookmarks = fm_bookmarks_dup();
  l = fm_bookmarks_get_all(bookmarks);
  for(; l; l = l->next) {
    FmBookmarkItem* bm_item = (FmBookmarkItem*)l->data;
    item = new Item("folder", QString::fromUtf8(bm_item->name));
    rootItem->appendRow(item);
  }
}

PlacesModel::~PlacesModel() {
  if(bookmarks);
    g_object_unref(bookmarks);
  if(volumeMonitor)
    g_object_unref(volumeMonitor);
}

#if 0
void PlacesModel::addItem(GVolume* volume, FmFileInfoJob* job) {
}

void PlacesModel::addMount(GMount* mount, FmFileInfoJob* job) {
}

void PlacesModel::addItem(GObject* volume_or_mount, FmFileInfoJob* job) {
    if(G_IS_VOLUME(volume_or_mount))
    {
        tp = gtk_tree_row_reference_get_path(model->separator);
        item = add_new_item(GTK_LIST_STORE(model), FM_PLACES_ITEM_VOLUME, &it, tp);
        gtk_tree_path_free(tp);
        item->volume = G_VOLUME(g_object_ref(volume_or_mount));
    }
    else if(G_IS_MOUNT(volume_or_mount))
    {
        tp = gtk_tree_row_reference_get_path(model->separator);
        item = add_new_item(GTK_LIST_STORE(model), FM_PLACES_ITEM_MOUNT, &it, tp);
        gtk_tree_path_free(tp);
        item->mount = G_MOUNT(g_object_ref(volume_or_mount));
    }
    else
    {
        /* NOTE: this is impossible, unless a bug exists */
        return;
    }
    update_volume_or_mount(model, item, &it, job);
}

void PlacesModel::update_volume_or_mount(FmPlacesItem* item, GtkTreeIter* it, FmFileInfoJob* job)
{
    GIcon* gicon;
    char* name;
    GdkPixbuf* pix;
    GMount* mount;
    FmPath* path;

    if(item->type == FM_PLACES_ITEM_VOLUME)
    {
        name = g_volume_get_name(item->volume);
        gicon = g_volume_get_icon(item->volume);
        mount = g_volume_get_mount(item->volume);
    }
    else if(G_LIKELY(item->type == FM_PLACES_ITEM_MOUNT))
    {
        name = g_mount_get_name(item->mount);
        gicon = g_mount_get_icon(item->mount);
        mount = g_object_ref(item->mount);
    }
    else
        return; /* FIXME: is it possible? */

    if(item->icon)
        fm_icon_unref(item->icon);
    item->icon = fm_icon_from_gicon(gicon);
    g_object_unref(gicon);

    if(mount)
    {
        GFile* gf = g_mount_get_root(mount);
        path = fm_path_new_for_gfile(gf);
        g_object_unref(gf);
        g_object_unref(mount);
        item->mounted = TRUE;
    }
    else
    {
        path = NULL;
        item->mounted = FALSE;
    }

    if(!fm_path_equal(fm_file_info_get_path(item->fi), path))
    {
        fm_file_info_set_path(item->fi, path);
        if(path)
        {
            if(job)
                fm_file_info_job_add(job, path);
            else
            {
                job = fm_file_info_job_new(NULL, FM_FILE_INFO_JOB_FOLLOW_SYMLINK);
                model->jobs = g_slist_prepend(model->jobs, job);
                g_signal_connect(job, "finished", G_CALLBACK(on_file_info_job_finished), model);
                fm_job_run_async(FM_JOB(job));
            }
            fm_path_unref(path);
        }
    }

    pix = fm_pixbuf_from_icon(item->icon, fm_config->pane_icon_size);
    gtk_list_store_set(GTK_LIST_STORE(model), it, FM_PLACES_MODEL_COL_ICON, pix, FM_PLACES_MODEL_COL_LABEL, name, -1);
    g_object_unref(pix);
    g_free(name);
}

#endif

/*

QModelIndex PlacesModel::index(int row, int column, const QModelIndex& parent) const {
  return QAbstractListModel::index(row, column, parent);
}

bool PlacesModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
  return QAbstractListModel::dropMimeData(data, action, row, column, parent);
}

QVariant PlacesModel::data(const QModelIndex& index, int role) const {

}

int PlacesModel::rowCount(const QModelIndex& parent) const {

}

PlacesModel::~QAbstractQStandardItemModel() {

}

QMimeData* PlacesModel::mimeData(const QModelIndexList& indexes) const {
  return QAbstractQStandardItemModel::mimeData(indexes);
}

Qt::DropActions PlacesModel::supportedDropActions() const {
  return QAbstractQStandardItemModel::supportedDropActions();
}
*/

#include "placesmodel.moc"
