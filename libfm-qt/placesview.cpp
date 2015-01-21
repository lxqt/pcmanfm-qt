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
#include "placesmodelitem.h"
#include "mountoperation.h"
#include "fileoperation.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QDebug>
#include <QGuiApplication>

using namespace Fm;

PlacesView::PlacesView(QWidget* parent):
  QTreeView(parent),
  currentPath_(NULL) {
  setRootIsDecorated(false);
  setHeaderHidden(true);
  setIndentation(12);

  connect(this, &QTreeView::clicked, this, &PlacesView::onClicked);
  connect(this, &QTreeView::pressed, this, &PlacesView::onPressed);

  setIconSize(QSize(24, 24));

  // FIXME: we may share this model amont all views
  model_ = new PlacesModel(this);
  setModel(model_);
  QHeaderView* headerView = header();
  headerView->setSectionResizeMode(0, QHeaderView::Stretch);
  headerView->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  headerView->setStretchLastSection(false);
  expandAll();

  // FIXME: is there any better way to make the first column span the whole row?
  setFirstColumnSpanned(0, QModelIndex(), true); // places root
  setFirstColumnSpanned(1, QModelIndex(), true); // devices root
  setFirstColumnSpanned(2, QModelIndex(), true); // bookmarks root

  // the 2nd column is for the eject buttons
  setSelectionBehavior(QAbstractItemView::SelectRows); // FIXME: why this does not work?
  setAllColumnsShowFocus(false);

  setAcceptDrops(true);
  setDragEnabled(true);
}

PlacesView::~PlacesView() {
  if(currentPath_)
    fm_path_unref(currentPath_);
  // qDebug("delete PlacesView");
}

void PlacesView::activateRow(int type, const QModelIndex& index) {
  if(!index.parent().isValid()) // ignore root items
    return;
  PlacesModelItem* item = static_cast<PlacesModelItem*>(model_->itemFromIndex(index));
  if(item) {
    FmPath* path = item->path();
    if(!path) {
      // check if mounting volumes is needed
      if(item->type() == PlacesModelItem::Volume) {
        PlacesModelVolumeItem* volumeItem = static_cast<PlacesModelVolumeItem*>(item);
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
      Q_EMIT chdirRequested(type, path);
    }
  }
}

// mouse button pressed
void PlacesView::onPressed(const QModelIndex& index) {
  // if middle button is pressed
  if(QGuiApplication::mouseButtons() & Qt::MiddleButton) {
    activateRow(1, index);
  }
}

void PlacesView::onEjectButtonClicked(PlacesModelItem* item) {
  // The eject button is clicked for a device item (volume or mount)
  if(item->type() == PlacesModelItem::Volume) {
    PlacesModelVolumeItem* volumeItem = static_cast<PlacesModelVolumeItem*>(item);
    MountOperation* op = new MountOperation(true, this);
    if(volumeItem->canEject()) // do eject if applicable
      op->eject(volumeItem->volume());
    else // otherwise, do unmount instead
      op->unmount(volumeItem->volume());
  }
  else if(item->type() == PlacesModelItem::Mount) {
    PlacesModelMountItem* mountItem = static_cast<PlacesModelMountItem*>(item);
    MountOperation* op = new MountOperation(true, this);
    op->unmount(mountItem->mount());
  }
  qDebug("PlacesView::onEjectButtonClicked");
}

void PlacesView::onClicked(const QModelIndex& index) {
  if(!index.parent().isValid()) // ignore root items
    return;

  if(index.column() == 0) {
    activateRow(0, index);
  }
  else if(index.column() == 1) { // column 1 contains eject buttons of the mounted devices
    if(index.parent() == model_->devicesRoot->index()) { // this is a mounted device
      // the eject button is clicked
      QModelIndex itemIndex = index.sibling(index.row(), 0); // the real item is at column 0
      PlacesModelItem* item = static_cast<PlacesModelItem*>(model_->itemFromIndex(itemIndex));
      if(item) {
        // eject the volume or the mount
        onEjectButtonClicked(item);
      }
    }
  }
}

void PlacesView::setCurrentPath(FmPath* path) {
  if(currentPath_)
    fm_path_unref(currentPath_);
  if(path) {
    currentPath_ = fm_path_ref(path);
    // TODO: search for item with the path in model_ and select it.
    PlacesModelItem* item = model_->itemFromPath(currentPath_);
    if(item) {
      selectionModel()->select(item->index(), QItemSelectionModel::SelectCurrent|QItemSelectionModel::Rows);
    }
    else
      clearSelection();
  }
  else {
    currentPath_ = NULL;
    clearSelection();
  }
}


void PlacesView::dragMoveEvent(QDragMoveEvent* event) {
  QTreeView::dragMoveEvent(event);
  /*
  QModelIndex index = indexAt(event->pos());
  if(event->isAccepted() && index.isValid() && index.parent() == model_->bookmarksRoot->index()) {
    if(dropIndicatorPosition() != OnItem) {
      event->setDropAction(Qt::LinkAction);
      event->accept();
    }
  }
  */
}

void PlacesView::dropEvent(QDropEvent* event) {
  QTreeView::dropEvent(event);
}

void PlacesView::onEmptyTrash() {
  FmPathList* files = fm_path_list_new();
  fm_path_list_push_tail(files, fm_path_get_trash());
  Fm::FileOperation::deleteFiles(files);
  fm_path_list_unref(files);
}

void PlacesView::onDeleteBookmark() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  if(!action->index().isValid())
    return;
  PlacesModelBookmarkItem* item = static_cast<PlacesModelBookmarkItem*>(model_->itemFromIndex(action->index()));
  FmBookmarkItem* bookmarkItem = item->bookmark();
  FmBookmarks* bookmarks = fm_bookmarks_dup();
  fm_bookmarks_remove(bookmarks, bookmarkItem);
  g_object_unref(bookmarks);
}

void PlacesView::onRenameBookmark() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  if(!action->index().isValid())
    return;
  PlacesModelBookmarkItem* item = static_cast<PlacesModelBookmarkItem*>(model_->itemFromIndex(action->index()));
  setFocus();
  setCurrentIndex(item->index());
  edit(item->index());
/*
  FmBookmarkItem* bookmarkItem = item->bookmark();
  FmBookmarks* bookmarks = fm_bookmarks_dup();
  // TODO: rename bookmark
  // fm_bookmarks_rename(bookmarks, bookmarkItem);
  g_object_unref(bookmarks);
*/
}

void PlacesView::onMountVolume() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  if(!action->index().isValid())
    return;
  PlacesModelVolumeItem* item = static_cast<PlacesModelVolumeItem*>(model_->itemFromIndex(action->index()));
  MountOperation* op = new MountOperation(true, this);
  op->mount(item->volume());
  op->wait();
}

void PlacesView::onUnmountVolume() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  if(!action->index().isValid())
    return;
  PlacesModelVolumeItem* item = static_cast<PlacesModelVolumeItem*>(model_->itemFromIndex(action->index()));
  GMount* mount = NULL;
  MountOperation* op = new MountOperation(true, this);
  op->unmount(item->volume());
  op->wait();
}

void PlacesView::onUnmountMount() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  if(!action->index().isValid())
    return;
  PlacesModelMountItem* item = static_cast<PlacesModelMountItem*>(model_->itemFromIndex(action->index()));
  GMount* mount = item->mount();
  MountOperation* op = new MountOperation(true, this);
  op->unmount(mount);
  op->wait();
}

void PlacesView::onEjectVolume() {
  PlacesModel::ItemAction* action = static_cast<PlacesModel::ItemAction*>(sender());
  if(!action->index().isValid())
    return;
  PlacesModelVolumeItem* item = static_cast<PlacesModelVolumeItem*>(model_->itemFromIndex(action->index()));
  MountOperation* op = new MountOperation(true, this);
  op->eject(item->volume());
  op->wait();
}

void PlacesView::contextMenuEvent(QContextMenuEvent* event) {
  QModelIndex index = indexAt(event->pos());
  if(index.isValid() && index.parent().isValid()) {
    QMenu* menu = NULL;
    QAction* action;
    PlacesModelItem* item = static_cast<PlacesModelItem*>(model_->itemFromIndex(index));
    switch(item->type()) {
      case PlacesModelItem::Places: {
        FmPath* path = item->path();
        if(path && fm_path_equal(fm_path_get_trash(), path)) {
          menu = new QMenu(this);
          action = new PlacesModel::ItemAction(item->index(), tr("Empty Trash"), menu);
          connect(action, SIGNAL(triggered(bool)), SLOT(onEmptyTrash()));
          menu->addAction(action);
        }
        break;
      }
      case PlacesModelItem::Bookmark: {
        // create context menu for bookmark item
        menu = new QMenu(this);
        action = new PlacesModel::ItemAction(item->index(), tr("Rename"), menu);
        connect(action, SIGNAL(triggered(bool)), SLOT(onRenameBookmark()));
        menu->addAction(action);
        action = new PlacesModel::ItemAction(item->index(), tr("Delete"), menu);
        connect(action, SIGNAL(triggered(bool)), SLOT(onDeleteBookmark()));
        menu->addAction(action);
        break;
      }
      case PlacesModelItem::Volume: {
        PlacesModelVolumeItem* volumeItem = static_cast<PlacesModelVolumeItem*>(item);
        menu = new QMenu(this);

        if(volumeItem->isMounted()) {
          action = new PlacesModel::ItemAction(item->index(), tr("Unmount"), menu);
          connect(action, SIGNAL(triggered(bool)), SLOT(onUnmountVolume()));
        }
        else {
          action = new PlacesModel::ItemAction(item->index(), tr("Mount"), menu);
          connect(action, SIGNAL(triggered(bool)), SLOT(onMountVolume()));
        }
        menu->addAction(action);

        if(volumeItem->canEject()) {
          action = new PlacesModel::ItemAction(item->index(), tr("Eject"), menu);
          connect(action, SIGNAL(triggered(bool)), SLOT(onEjectVolume()));
          menu->addAction(action);
        }
        break;
      }
      case PlacesModelItem::Mount: {
        menu = new QMenu(this);
        action = new PlacesModel::ItemAction(item->index(), tr("Unmount"), menu);
        connect(action, SIGNAL(triggered(bool)), SLOT(onUnmountMount()));
        menu->addAction(action);
        break;
      }
    }
    if(menu) {
      menu->popup(mapToGlobal(event->pos()));
      connect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));
    }
  }
}
