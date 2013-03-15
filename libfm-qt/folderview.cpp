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


#include "folderview.h"
#include "foldermodel.h"
#include <QHeaderView>
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include "proxyfoldermodel.h"
#include "folderitemdelegate.h"
#include "dndactionmenu.h"

using namespace Fm;

FolderView::FolderView(ViewMode _mode, QWidget* parent):
  QWidget(parent),
  view(NULL),
  mode((ViewMode)0),
  model_(NULL) {

  iconSize_[IconMode - FirstViewMode] = QSize(48, 48);
  iconSize_[CompactMode - FirstViewMode] = QSize(24, 24);
  iconSize_[ThumbnailMode - FirstViewMode] = QSize(128, 128);
  iconSize_[DetailedListMode - FirstViewMode] = QSize(24, 24);

  QVBoxLayout* layout = new QVBoxLayout();
  layout->setMargin(0);
  setLayout(layout);

  setViewMode(_mode);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

FolderView::~FolderView() {
}

void FolderView::ListView::startDrag(Qt::DropActions supportedActions) {
  if(movement() != Static)
    QListView::startDrag(supportedActions);
  else
    QAbstractItemView::startDrag(supportedActions);
}

void FolderView::ListView::mousePressEvent(QMouseEvent* event) {
  QListView::mousePressEvent(event);
  static_cast<FolderView*>(parent())->childMousePressEvent(event);
}

// NOTE:
// QListView has a problem which I consider a bug or a design flaw.
// When you set movement property to Static, theoratically the icons
// should not be movable. However, if you turned on icon mode,
// the icons becomes freely movable despite the value of movement is Static.
// To overcome this bug, we override all drag handling methods, and
// call QAbstractItemView directly, bypassing QListView.
// In this way, we can workaround the buggy behavior.
// The drag handlers of QListView basically does the same things
// as its parent QAbstractItemView, but it also stores the currently
// dragged item and paint them in the view as needed.
// TODO: I really should file a bug report to Qt developers.

void FolderView::ListView::dragEnterEvent(QDragEnterEvent* event) {
  if(movement() != Static)
    QListView::dragEnterEvent(event);
  else
    QAbstractItemView::dragEnterEvent(event);
  qDebug("dragEnterEvent");
  //static_cast<FolderView*>(parent())->childDragEnterEvent(event);
}

void FolderView::ListView::dragLeaveEvent(QDragLeaveEvent* e) {
  if(movement() != Static)
    QListView::dragLeaveEvent(e);
  else
    QAbstractItemView::dragLeaveEvent(e);
  static_cast<FolderView*>(parent())->childDragLeaveEvent(e);
}

void FolderView::ListView::dragMoveEvent(QDragMoveEvent* e) {
  if(movement() != Static)
    QListView::dragMoveEvent(e);
  else
    QAbstractItemView::dragMoveEvent(e);
  static_cast<FolderView*>(parent())->childDragMoveEvent(e);
}

void FolderView::ListView::dropEvent(QDropEvent* e) {

  static_cast<FolderView*>(parent())->childDropEvent(e);

  if(movement() != Static)
    QListView::dropEvent(e);
  else
    QAbstractItemView::dropEvent(e);
}


void FolderView::TreeView::mousePressEvent(QMouseEvent* event) {
  QTreeView::mousePressEvent(event);
  static_cast<FolderView*>(parent())->childMousePressEvent(event);
}

void FolderView::TreeView::dragEnterEvent(QDragEnterEvent* event) {
  QTreeView::dragEnterEvent(event);
  static_cast<FolderView*>(parent())->childDragEnterEvent(event);
}

void FolderView::TreeView::dragLeaveEvent(QDragLeaveEvent* e) {
  QTreeView::dragLeaveEvent(e);
  static_cast<FolderView*>(parent())->childDragLeaveEvent(e);
}

void FolderView::TreeView::dragMoveEvent(QDragMoveEvent* e) {
  QTreeView::dragMoveEvent(e);
  static_cast<FolderView*>(parent())->childDragMoveEvent(e);
}

void FolderView::TreeView::dropEvent(QDropEvent* e) {
  static_cast<FolderView*>(parent())->childDropEvent(e);
  QTreeView::dropEvent(e);
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
  QSize iconSize = iconSize_[mode - FirstViewMode];

  if(mode == DetailedListMode) {
    TreeView* treeView = new TreeView(this);
    view = treeView;
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    treeView->setAllColumnsShowFocus(false);

    // FIXME: why this doesn't work?
    treeView->header()->setResizeMode(0, QHeaderView::Stretch); // QHeaderView::ResizeToContents);
    // treeView->setSelectionBehavior(QAbstractItemView::SelectItems);
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
    listView->setItemDelegateForColumn(FolderModel::ColumnFileName, delegate);
    // FIXME: should we expose the delegate?
    listView->setMovement(QListView::Static);
    listView->setResizeMode(QListView::Adjust);
    listView->setWrapping(true);
    switch(mode) {
      case IconMode: {
        listView->setViewMode(QListView::IconMode);
        // listView->setGridSize(QSize(iconSize.width() * 1.6, iconSize.height() * 2));
        listView->setGridSize(QSize(80, 100));
        listView->setWordWrap(true);
        listView->setFlow(QListView::LeftToRight);
        break;
      }
      case CompactMode: {
        listView->setViewMode(QListView::ListMode);
        listView->setGridSize(QSize());
        listView->setWordWrap(false);
        listView->setFlow(QListView::QListView::TopToBottom);
        break;
      }
      case ThumbnailMode: {
        listView->setViewMode(QListView::IconMode);
        listView->setGridSize(QSize(160, 160));
        listView->setWordWrap(true);
        listView->setFlow(QListView::LeftToRight);
        break;
      }
    }
  }
  if(view) {
    connect(view, SIGNAL(activated(QModelIndex)), SLOT(onItemActivated(QModelIndex)));

    view->setContextMenuPolicy(Qt::NoContextMenu); // defer the context menu handling to parent widgets
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setIconSize(iconSize);

    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout()->addWidget(view);

    // enable dnd
    view->setDragEnabled(true);
    view->setAcceptDrops(true);
    view->setDragDropMode(QAbstractItemView::DragDrop);
    view->setDropIndicatorShown(true);

    if(model_) {
      // FIXME: preserve selections
      model_->setThumbnailSize(iconSize.width());
      view->setModel(model_);
    }
  }
}

void FolderView::setIconSize(ViewMode mode, QSize size) {
  Q_ASSERT(mode >= FirstViewMode && mode <= LastViewMode);
  iconSize_[mode - FirstViewMode] = size;
  if(viewMode() == mode)
    view->setIconSize(size);
}

QSize FolderView::iconSize(ViewMode mode) const {
  Q_ASSERT(mode >= FirstViewMode && mode <= LastViewMode);
  return iconSize_[mode - FirstViewMode];
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
  if(view) {
    view->setModel(model);
    QSize iconSize = iconSize_[mode - FirstViewMode];
    model->setThumbnailSize(iconSize.width());
  }
  if(model_)
    delete model_;
  model_ = model;
}

void FolderView::contextMenuEvent(QContextMenuEvent* event) {
  QWidget::contextMenuEvent(event);
  QPoint pos = event->pos();
  QPoint pos2 = view->mapFromParent(pos);
  emitClickedAt(ContextMenuClick, pos2);
}

void FolderView::childMousePressEvent(QMouseEvent* event) {
  // called from mousePressEvent() of chld view
  if(event->button() == Qt::MiddleButton) {
    QPoint pos = event->pos();
    emitClickedAt(MiddleClick, pos);
  }
}

void FolderView::emitClickedAt(ClickType type, QPoint& pos) {
  QPoint viewport_pos = view->viewport()->mapFromParent(pos);
  // indexAt() needs a point in "viewport" coordinates.
  QModelIndex index = view->indexAt(viewport_pos);
  if(index.isValid()) {
    QVariant data = index.data(FolderModel::FileInfoRole);
    FmFileInfo* info = reinterpret_cast<FmFileInfo*>(data.value<void*>());
    Q_EMIT clicked(type, info);
  }
  else {
    // FIXME: should we show popup menu for the selected files instead
    // if there are selected files?
    if(type == ContextMenuClick) {
      // clear current selection if clicked outside selected files
      view->clearSelection();
      Q_EMIT clicked(type, NULL);
    }
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
  if(model_) {
    QItemSelectionModel* selModel = view->selectionModel();
    int rows = model_->rowCount();
    for(int row = 0; row < rows; ++row) {
      QModelIndex index = model_->index(row, 0);
      selModel->select(index, QItemSelectionModel::Toggle|QItemSelectionModel::Rows);
    }
  }
}

void FolderView::childDragEnterEvent(QDragEnterEvent* event) {
  qDebug("drag enter");
  if(event->mimeData()->hasFormat("text/uri-list")) {
    event->accept();
  }
  else
    event->ignore();
}

void FolderView::childDragLeaveEvent(QDragLeaveEvent* e) {
  qDebug("drag leave");
  e->accept();
}

void FolderView::childDragMoveEvent(QDragMoveEvent* e) {
  qDebug("drag move");
}

void FolderView::childDropEvent(QDropEvent* e) {
  qDebug("drop");
  if(e->keyboardModifiers() == Qt::NoModifier) {
    // if no key modifiers are used, popup a menu
    // to ask the user for the action he/she wants to perform.
    Qt::DropAction action = DndActionMenu::askUser(QCursor::pos());
    e->setDropAction(action);
  }
}

#include "folderview.moc"
