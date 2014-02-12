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

DirTreeModel::DirTreeModel(QObject* parent) {
}

DirTreeModel::~DirTreeModel() {
}

// QAbstractItemModel implementation

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
  return 0;
}

QModelIndex DirTreeModel::parent(const QModelIndex& child) const {
  DirTreeModelItem* item = itemFromIndex(child);
  if(item && item->parent_) {
    item = item->parent_; // go to parent item
    const QList<DirTreeModelItem*>& items = item->parent_ ? item->parent_->children_ : rootItems_;
    int row = items.indexOf(item); // this is Q(n) and may be slow :-(
    if(row >= 0)
      return createIndex(row, 1, (void*)item);
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
  // DirTreeModelItem* item = itemFromIndex(parent);
  // return item ? !item->children_.isEmpty() : NULL;
  return true;
}

QModelIndex DirTreeModel::indexFromItem(DirTreeModelItem* item) const {
  Q_ASSERT(item);
  const QList<DirTreeModelItem*>& items = item->parent_ ? item->parent_->children_ : rootItems_;
  int row = items.indexOf(item);
  if(row >= 0)
    return createIndex(row, 1, (void*)item);
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

/* Add file info to parent node to proper position.
 * GtkTreePath tp is the tree path of parent node. */
DirTreeModelItem* DirTreeModel::insertFileInfo(FmFileInfo* fi, Fm::DirTreeModelItem* parentItem) {
  qDebug() << "insertFileInfo: " << fm_file_info_get_disp_name(fi);
  DirTreeModelItem* item = new DirTreeModelItem(fi, this);
  insertItem(item, parentItem);
  return item;
}

// find a good position to insert the new item
int DirTreeModel::insertItem(DirTreeModelItem* newItem, DirTreeModelItem* parentItem) {
  if(!parentItem) {
    // FIXME: handle root items
    return -1;
  }
  const char* new_key = fm_file_info_get_collate_key(newItem->fileInfo_);
  int pos = 0;
  QList<DirTreeModelItem*>::iterator it;
  for(it = parentItem->children_.begin(); it != parentItem->children_.end(); ++it) {
    DirTreeModelItem* child = *it;
    if(G_UNLIKELY(!child->fileInfo_))
      continue;
    const char* key = fm_file_info_get_collate_key(child->fileInfo_);
    if(strcmp(new_key, key) <= 0)
      break;
    ++pos;
  }
  // inform the world that we're about to insert the item
  QModelIndex parentIndex = indexFromItem(parentItem);
  beginInsertRows(parentIndex, pos, pos);
  newItem->parent_ = parentItem;
  parentItem->children_.insert(it, newItem);
  endInsertRows();

  return pos;
}

void DirTreeModel::loadRow(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  Q_ASSERT(item);
  if(item)
    item->loadFolder();
}

void DirTreeModel::unloadRow(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  if(item)
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
  return item ? fm_file_info_get_path(item->fileInfo_) : NULL;
}

const char* DirTreeModel::dispName(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? fm_file_info_get_disp_name(item->fileInfo_) : NULL;
}

void DirTreeModel::setShowHidden(bool show_hidden) {
  showHidden_ = show_hidden;
}


} // namespace Fm
