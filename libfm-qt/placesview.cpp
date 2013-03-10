/*

    Copyright (C) 2012  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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


#include "placesview.h"
#include "placesmodel.h"
#include "mountoperation.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QHeaderView>

using namespace Fm;

PlacesView::PlacesView(QWidget* parent):
  QTreeView(parent),
  currentPath_(NULL) {
  setRootIsDecorated(false);
  setHeaderHidden(true);
  setIndentation(12);
  
  connect(this, SIGNAL(clicked(QModelIndex)), SLOT(onClicked(QModelIndex)));
  
  setIconSize(QSize(24, 24));
  
  model_ = new PlacesModel(this);
  setModel(model_);
  QHeaderView* headerView = header();
  headerView->setResizeMode(0, QHeaderView::Stretch);
  // headerView->setResizeMode(1, QHeaderView::Fixed);
  headerView->setStretchLastSection(false);
  expandAll();
}

PlacesView::~PlacesView() {
  if(currentPath_)
    fm_path_unref(currentPath_);
}

void PlacesView::onClicked(const QModelIndex& index) {
  if(!index.parent().isValid()) // ignore root items
    return;

  PlacesModel::Item* item = static_cast<PlacesModel::Item*>(model_->itemFromIndex(index));
  if(item) {
    FmPath* path = item->path();
    if(!path) {
      // check if mounting volumes is needed
      if(item->type() == PlacesModel::ItemTypeVolume) {
        PlacesModel::VolumeItem* volumeItem = static_cast<PlacesModel::VolumeItem*>(item);
        if(!volumeItem->isMounted()) {
          // Mount the volume
          GVolume* volume = volumeItem->volume();
          MountOperation* op = new MountOperation(true, this);
          op->mount(volume);
          // connect(op, SIGNAL(finished(GError*)), SLOT(onMountOperationFinished(GError*)));
          // blocking here until the mount operation is finished?

          // FIXME: update status of the volume after mount is finished!!
          if(!op->wait())
            return;
          path = item->path();
        }
      }
    }
    if(path) {
      Q_EMIT chdirRequested(0, path);
    }
  }
}

void PlacesView::setCurrentPath(FmPath* path) {
  if(currentPath_)
    fm_path_unref(currentPath_);
  if(path) {
    currentPath_ = fm_path_ref(path);
    // TODO: search for item with the path in model_ and select it.
    
  }
  else
    currentPath_ = NULL;
}


void PlacesView::dragMoveEvent(QDragMoveEvent* event) {
  QTreeView::dragMoveEvent(event);
}

void PlacesView::dropEvent(QDropEvent* event) {
  QTreeView::dropEvent(event);
}

void PlacesView::onDeleteBookmark() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  PlacesModel::BookmarkItem* item = static_cast<PlacesModel::BookmarkItem*>(action->item());
  FmBookmarkItem* bookmarkItem = item->bookmark();
  FmBookmarks* bookmarks = fm_bookmarks_dup();
  fm_bookmarks_remove(bookmarks, bookmarkItem);
  g_object_unref(bookmarks);
}

void PlacesView::onRenameBookmark() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  PlacesModel::BookmarkItem* item = static_cast<PlacesModel::BookmarkItem*>(action->item());
  FmBookmarkItem* bookmarkItem = item->bookmark();
  FmBookmarks* bookmarks = fm_bookmarks_dup();
  // TODO: rename bookmark
  // fm_bookmarks_rename(bookmarks, bookmarkItem);
  g_object_unref(bookmarks);
}

void PlacesView::onMountVolume() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  PlacesModel::VolumeItem* item = static_cast<PlacesModel::VolumeItem*>(action->item());
  MountOperation* op = new MountOperation(true, this);
  op->mount(item->volume());
  op->wait();
}

void PlacesView::onUnmountVolume() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  GMount* mount = NULL;
  PlacesModel::VolumeItem* item = static_cast<PlacesModel::VolumeItem*>(action->item());
  MountOperation* op = new MountOperation(true, this);
  op->unmount(item->volume());
  op->wait();
}

void PlacesView::onUnmountMount() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  PlacesModel::MountItem* item = static_cast<PlacesModel::MountItem*>(action->item());
  GMount* mount = item->mount();
  MountOperation* op = new MountOperation(true, this);
  op->unmount(mount);
  op->wait();
}

void PlacesView::onEjectVolume() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  PlacesModel::VolumeItem* item = static_cast<PlacesModel::VolumeItem*>(action->item());
  MountOperation* op = new MountOperation(true, this);
  op->eject(item->volume());
  op->wait();
}

void PlacesView::contextMenuEvent(QContextMenuEvent* event) {
  QModelIndex index = indexAt(event->pos());
  if(index.isValid() && index.parent().isValid()) {
    QMenu* menu;
    QAction* action;
    PlacesModel::Item* item = static_cast<PlacesModel::Item*>(model_->itemFromIndex(index));
    switch(item->type()) {
      case PlacesModel::ItemTypeBookmark: {
        // create context menu for bookmark item
        menu = new QMenu(this);
        action = new PlacesModel::ItemAction(item, tr("Rename"), menu);
        connect(action, SIGNAL(triggered(bool)), SLOT(onRenameBookmark()));
        menu->addAction(action);
        action = new PlacesModel::ItemAction(item, tr("Delete"), menu);
        connect(action, SIGNAL(triggered(bool)), SLOT(onDeleteBookmark()));
        menu->addAction(action);
        break;
      }
      case PlacesModel::ItemTypeVolume: {
        PlacesModel::VolumeItem* volumeItem = static_cast<PlacesModel::VolumeItem*>(item);
        menu = new QMenu(this);

        if(volumeItem->isMounted()) {
          action = new PlacesModel::ItemAction(item, tr("Unmount"), menu);
          connect(action, SIGNAL(triggered(bool)), SLOT(onUnmountVolume()));
        }
        else {
          action = new PlacesModel::ItemAction(item, tr("Mount"), menu);
          connect(action, SIGNAL(triggered(bool)), SLOT(onMountVolume()));
        }
        menu->addAction(action);

        if(volumeItem->canEject()) {
          action = new PlacesModel::ItemAction(item, tr("Eject"), menu);
          connect(action, SIGNAL(triggered(bool)), SLOT(onEjectVolume()));
          menu->addAction(action);
        }
        break;
      }
      case PlacesModel::ItemTypeMount: {
        menu = new QMenu(this);
        action = new PlacesModel::ItemAction(item, tr("Unmount"), menu);
        connect(action, SIGNAL(triggered(bool)), SLOT(onUnmountMount()));
        menu->addAction(action);
        break;
      }
      default:
        // QTreeView::contextMenuEvent(event);
        return;
    }
    menu->popup(mapToGlobal(event->pos()));
  }
}



#include "placesview.moc"
