/*
 * Copyright 2014 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "dirtreemodel.h"
#include "dirtreemodelitem.h"
#include <QDebug>

namespace Fm {

DirTreeModel::DirTreeModel(QObject* parent):
  showHidden_(false) {
}

DirTreeModel::~DirTreeModel() {
}

// QAbstractItemModel implementation

Qt::ItemFlags DirTreeModel::flags(const QModelIndex& index) const {
  DirTreeModelItem* item = itemFromIndex(index);
  if(item && item->isPlaceHolder())
    return Qt::ItemIsEnabled;
  return QAbstractItemModel::flags(index);
}

QVariant DirTreeModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.column() > 1) {
    return QVariant();
  }
  DirTreeModelItem* item = itemFromIndex(index);
  if(item) {
    FmFileInfo* info = item->fileInfo_;
    switch(role) {
    case Qt::ToolTipRole:
      return QVariant(item->displayName_);
    case Qt::DisplayRole:
      return QVariant(item->displayName_);
    case Qt::DecorationRole:
      return QVariant(item->icon_);
    case FileInfoRole:
      return qVariantFromValue((void*)info);
    }
  }
  return QVariant();
}

int DirTreeModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

int DirTreeModel::rowCount(const QModelIndex& parent) const {
  if(!parent.isValid())
    return rootItems_.count();
  DirTreeModelItem* item = itemFromIndex(parent);
  if(item)
    return item->children_.count();
}

QModelIndex DirTreeModel::parent(const QModelIndex& child) const {
  DirTreeModelItem* item = itemFromIndex(child);
  if(item && item->parent_) {
    item = item->parent_; // go to parent item
    if(item) {
      const QList<DirTreeModelItem*>& items = item->parent_ ? item->parent_->children_ : rootItems_;
      int row = items.indexOf(item); // this is Q(n) and may be slow :-(
      if(row >= 0)
	return createIndex(row, 0, (void*)item);
    }
  }
  return QModelIndex();
}

QModelIndex DirTreeModel::index(int row, int column, const QModelIndex& parent) const {
  if(row >= 0 && column >= 0 && column == 0) {
    if(!parent.isValid()) { // root items
      if(row < rootItems_.count()) {
        const DirTreeModelItem* item = rootItems_.at(row);
        return createIndex(row, column, (void*)item);
      }
    }
    else { // child items
      DirTreeModelItem* parentItem = itemFromIndex(parent);
      if(row < parentItem->children_.count()) {
        const DirTreeModelItem* item = parentItem->children_.at(row);
        return createIndex(row, column, (void*)item);
      }
    }
  }
  return QModelIndex(); // invalid index
}

bool DirTreeModel::hasChildren(const QModelIndex& parent) const {
  DirTreeModelItem* item = itemFromIndex(parent);
  return item ? !item->isPlaceHolder() : true;
}

QModelIndex DirTreeModel::indexFromItem(DirTreeModelItem* item) const {
  Q_ASSERT(item);
  const QList<DirTreeModelItem*>& items = item->parent_ ? item->parent_->children_ : rootItems_;
  int row = items.indexOf(item);
  if(row >= 0)
    return createIndex(row, 0, (void*)item);
  return QModelIndex();
}

// public APIs
QModelIndex DirTreeModel::addRoot(FmFileInfo* root) {
  DirTreeModelItem* item = new DirTreeModelItem(root, this);
  int row = rootItems_.count();
  beginInsertRows(QModelIndex(), row, row);
  item->fileInfo_ = fm_file_info_ref(root);
  rootItems_.append(item);
  // add_place_holder_child_item(model, item_l, NULL, FALSE);
  endInsertRows();
  return QModelIndex();
}

DirTreeModelItem* DirTreeModel::itemFromIndex(const QModelIndex& index) const {
  return reinterpret_cast<DirTreeModelItem*>(index.internalPointer());
}

QModelIndex DirTreeModel::indexFromPath(FmPath* path) const {
  DirTreeModelItem* item = itemFromPath(path);
  return item ? item->index() : QModelIndex();
}

DirTreeModelItem* DirTreeModel::itemFromPath(FmPath* path) const {
  Q_FOREACH(DirTreeModelItem* item, rootItems_) {
    if(item->fileInfo_ && fm_path_equal(path, fm_file_info_get_path(item->fileInfo_))) {
      return item;
    }
    else {
      DirTreeModelItem* child = item->childFromPath(path, true);
      if(child)
	return child;
    }
  }
  return NULL;
}


void DirTreeModel::loadRow(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  Q_ASSERT(item);
  if(item && !item->isPlaceHolder())
    item->loadFolder();
}

void DirTreeModel::unloadRow(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  if(item && !item->isPlaceHolder())
    item->unloadFolder();
}

bool DirTreeModel::isLoaded(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? item->loaded_ : false;
}

QIcon DirTreeModel::icon(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? item->icon_ : QIcon();
}

FmFileInfo* DirTreeModel::fileInfo(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? item->fileInfo_ : NULL;
}

FmPath* DirTreeModel::filePath(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item && item->fileInfo_ ? fm_file_info_get_path(item->fileInfo_) : NULL;
}

QString DirTreeModel::dispName(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? item->displayName_ : QString();
}

void DirTreeModel::setShowHidden(bool show_hidden) {
  showHidden_ = show_hidden;
  Q_FOREACH(DirTreeModelItem* item, rootItems_) {
    item->setShowHidden(show_hidden);
  }
}


} // namespace Fm
