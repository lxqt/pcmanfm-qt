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

#include <iostream>

using namespace Fm;

FolderView::FolderView(ViewMode _mode, QWidget* parent):
  QWidget(parent),
  view(NULL),
  mode((ViewMode)0),
  model_(NULL) {

  QVBoxLayout* layout = new QVBoxLayout();
  layout->setMargin(0);
  setLayout(layout);

  setViewMode(_mode);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

FolderView::~FolderView() {
}

void FolderView::onItemActivated(QModelIndex index) {
  QVariant data = index.model()->data(index, FolderModel::FileInfoRole);
  FmFileInfo* info = (FmFileInfo*)data.value<void*>();
  if(info) {
    Q_EMIT clicked(ACTIVATED, info);
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
    QTreeView* treeView = new QTreeView(this);
    view = treeView;
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    treeView->header()->setResizeMode(0, QHeaderView::ResizeToContents); // QHeaderView::Stretch);

  }
  else {
    QListView* listView;
    if(view)
      listView = static_cast<QListView*>(view);
    else {
      listView = new QListView(this);
      view = listView;
    }
    listView->setMovement(QListView::Static);
    listView->setResizeMode(QListView::Adjust);
    listView->setWrapping(true);
    switch(mode) {
      case IconMode: {
	listView->setViewMode(QListView::IconMode);
	listView->setGridSize(QSize(80, 80));
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
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setIconSize(iconSize_);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout()->addWidget(view);
    if(model_) {
      // FIXME: preserve selections
      view->setModel(model_);
    }
  }
}

void FolderView::setIconSize(QSize size) {
  iconSize_ = size;
  if(model_) {
    // FIXME: this does not work.
    // there is no way to set icon size for the model now.
    // the icon size is now hard-coded in Fm::FolderModel.
  }
}

QSize FolderView::iconSize() {
  return iconSize_;
}

void FolderView::setGridSize(QSize size) {
  gridSize_ = size;
}

QSize FolderView::gridSize() {
  return gridSize_;
}

FolderView::ViewMode FolderView::viewMode() {
  return mode;
}

QAbstractItemView* FolderView::childView() {
  return view;
}

QAbstractItemModel* FolderView::model() {
  return model_;
}

void FolderView::setModel(QAbstractItemModel* model) {
  if(view)
    view->setModel(model);
  if(model_)
    delete model_;
  model_ = model;
}

#include "folderview.moc"
