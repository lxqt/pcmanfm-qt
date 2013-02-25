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


#include "folderview.h"
#include "foldermodel.h"
#include <QHeaderView>
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include "proxyfoldermodel.h"
#include "folderitemdelegate.h"

using namespace Fm;

FolderView::FolderView(ViewMode _mode, QWidget* parent):
  QWidget(parent),
  view(NULL),
  mode((ViewMode)0),
  model_(NULL),
  iconSize_(QSize(32,32)) {

  QVBoxLayout* layout = new QVBoxLayout();
  layout->setMargin(0);
  setLayout(layout);

  setViewMode(_mode);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

FolderView::~FolderView() {
}

void FolderView::ListView::mousePressEvent(QMouseEvent* event) {
  QListView::mousePressEvent(event);
  if(event->button() == Qt::MiddleButton) {
    QPoint pos = event->pos();
    static_cast<FolderView*>(parent())->emitClickedAt(MiddleClick, pos);
  }
}


void FolderView::TreeView::mousePressEvent(QMouseEvent* event) {
  QTreeView::mousePressEvent(event);
  if(event->button() == Qt::MiddleButton) {
    QPoint pos = event->pos();
    static_cast<FolderView*>(parent())->emitClickedAt(MiddleClick, pos);
  }
}


void FolderView::onItemActivated(QModelIndex index) {
  QVariant data = index.model()->data(index, FolderModel::FileInfoRole);
  FmFileInfo* info = (FmFileInfo*)data.value<void*>();
  if(info) {
    Q_EMIT clicked(ActivatedClick, info);
  }
}

void FolderView::setViewMode(ViewMode _mode) {
  if(_mode == mode) // if it's the same more, ignore
    return;
  // since only detailed list mode uses QTreeView, and others 
  // all use QListView, it's wise to preserve QListView when possible.
  if(view && mode == DetailedListMode || _mode == DetailedListMode) {
    delete view; // FIXME: no virtual dtor?
    view = NULL;
  }
  mode = _mode;

  if(mode == DetailedListMode) {
    TreeView* treeView = new TreeView(this);
    view = treeView;
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    treeView->setAllColumnsShowFocus(false);

    // FIXME: why this doesn't work?
    treeView->header()->setResizeMode(0, QHeaderView::Stretch); // QHeaderView::ResizeToContents);
    // treeView->setSelectionBehavior(QAbstractItemView::SelectItems);
    iconSize_ = QSize(fm_config->small_icon_size, fm_config->small_icon_size); // FIXME: should we use FmConfig?
  }
  else {
    ListView* listView;
    if(view)
      listView = static_cast<ListView*>(view);
    else {
      listView = new ListView(this);
      view = listView;
    }
    // set our own custom delegate
    FolderItemDelegate* delegate = new FolderItemDelegate(listView);
    listView->setItemDelegateForColumn(FolderModel::ColumnName, delegate);
    // FIXME: should we expose the delegate?

    listView->setDragDropMode(QAbstractItemView::DragDrop);
    listView->setMovement(QListView::Snap);
    listView->setResizeMode(QListView::Adjust);
    listView->setWrapping(true);
    
    switch(mode) {
      case IconMode: {
        listView->setViewMode(QListView::IconMode);
        listView->setGridSize(QSize(80, 100));
        listView->setSpacing(10);
        listView->setWordWrap(true);
        listView->setFlow(QListView::LeftToRight);

        iconSize_ = QSize(fm_config->big_icon_size, fm_config->big_icon_size);
        break;
      }
      case CompactMode: {
        listView->setViewMode(QListView::ListMode);
        listView->setGridSize(QSize());
        listView->setWordWrap(false);
        listView->setFlow(QListView::QListView::TopToBottom);

        iconSize_ = QSize(fm_config->small_icon_size, fm_config->small_icon_size);
        break;
      }
      case ThumbnailMode: {
        listView->setViewMode(QListView::IconMode);
        listView->setGridSize(QSize(160, 160));
        listView->setWordWrap(true);
        listView->setFlow(QListView::LeftToRight);

        iconSize_ = QSize(fm_config->thumbnail_size, fm_config->thumbnail_size);
        break;
      }
    }
  }
  if(view) {
    connect(view, SIGNAL(activated(QModelIndex)), SLOT(onItemActivated(QModelIndex)));

    view->setContextMenuPolicy(Qt::NoContextMenu); // defer the context menu handling to parent widgets
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setIconSize(iconSize_);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout()->addWidget(view);

    // enable dnd
    view->setDragEnabled(true);
    //view->setDragDropMode();
    view->setAcceptDrops(true);

    if(model_) {
      // FIXME: preserve selections
      view->setModel(model_);
    }
  }
}

void FolderView::setIconSize(QSize size) {
  iconSize_ = size;
  if(view)
    view->setIconSize(size);
}

QSize FolderView::iconSize() const {
  return iconSize_;
}

void FolderView::setGridSize(QSize size) {
  gridSize_ = size;
}

QSize FolderView::gridSize() const {
  return gridSize_;
}

FolderView::ViewMode FolderView::viewMode() const {
  return mode;
}

QAbstractItemView* FolderView::childView() const {
  return view;
}

ProxyFolderModel* FolderView::model() const {
  return model_;
}

void FolderView::setModel(ProxyFolderModel* model) {
  if(view)
    view->setModel(model);
  if(model_)
    delete model_;
  model_ = model;
}

void FolderView::contextMenuEvent(QContextMenuEvent* event) {
  QWidget::contextMenuEvent(event);
  QPoint pos = event->pos();
  g_print("%d, %d\n", pos.x(), pos.y());
  QPoint pos2 = view->mapFromParent(pos);
  emitClickedAt(ContextMenuClick, pos2);
  // g_print("MAPPED: %d, %d\n", pos2.x(), pos2.y());
}

void FolderView::emitClickedAt(ClickType type, QPoint& pos) {
  QPoint viewport_pos = view->viewport()->mapFromParent(pos);
  // indexAt() needs a point in "viewport" coordinates.
  QModelIndex index = view->indexAt(viewport_pos);
  g_print(" ROW: %d\n", index.row());
  if(index.isValid()) {
    QVariant data = index.data(FolderModel::FileInfoRole);
    FmFileInfo* info = reinterpret_cast<FmFileInfo*>(data.value<void*>());
    Q_EMIT clicked(type, info);
  }
  else {
    // FIXME: should we show popup menu for the selected files instead
    // if there are selected files?
    if(type == ContextMenuClick)
      Q_EMIT clicked(type, NULL);
  }
}

QModelIndexList FolderView::selectedRows(int column) const {
  QItemSelectionModel* selModel = selectionModel();
  if(selModel) {
    return selModel->selectedRows();
  }
  return QModelIndexList();
}

// This returns all selected "cells", which means all cells of the same row are returned.
QModelIndexList FolderView::selectedIndexes() const {
  QItemSelectionModel* selModel = selectionModel();
  if(selModel) {
    return selModel->selectedIndexes();
  }
  return QModelIndexList();
}

QItemSelectionModel* FolderView::selectionModel() const {
  return view ? view->selectionModel() : NULL;
}

FmPathList* FolderView::selectedFilePaths() const {
  if(model_) {
    QModelIndexList selIndexes = mode == DetailedListMode ? selectedRows() : selectedIndexes();
    if(!selIndexes.isEmpty()) {
      FmPathList* paths = fm_path_list_new();
      QModelIndexList::const_iterator it;
      for(it = selIndexes.begin(); it != selIndexes.end(); ++it) {
        FmFileInfo* file = model_->fileInfoFromIndex(*it);
        fm_path_list_push_tail(paths, fm_file_info_get_path(file));
      }
      return paths;
    }
  }
  return NULL;
}

FmFileInfoList* FolderView::selectedFiles() const {
  if(model_) {
    QModelIndexList selIndexes = mode == DetailedListMode ? selectedRows() : selectedIndexes();
    if(!selIndexes.isEmpty()) {
      FmFileInfoList* files = fm_file_info_list_new();
      QModelIndexList::const_iterator it;
      for(it = selIndexes.constBegin(); it != selIndexes.constEnd(); it++) {
        FmFileInfo* file = model_->fileInfoFromIndex(*it);
        fm_file_info_list_push_tail(files, file);
      }
      return files;
    }
  }
  return NULL;
}

void FolderView::invertSelection() {
  //TODO
}


#include "folderview.moc"
