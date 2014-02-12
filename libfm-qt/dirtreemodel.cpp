/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

namespace Fm {

DirTreeModel::DirTreeModel(QObject* parent) {
}

DirTreeModel::~DirTreeModel() {
}

// QAbstractItemModel implementation

QVariant DirTreeModel::data(const QModelIndex& index, int role) {
  return QVariant();
}

int DirTreeModel::columnCount(const QModelIndex& parent) {
  return 1;
}

int DirTreeModel::rowCount(const QModelIndex& parent) {
  if(!parent.isValid())
    return rootItems_.count();

  return 0;
}

QModelIndex DirTreeModel::parent(const QModelIndex& child) {
  return QModelIndex();
}

QModelIndex DirTreeModel::index(int row, int column, const QModelIndex& parent) {
  return QModelIndex();
}

// public APIs
QModelIndex DirTreeModel::addRoot(FmFileInfo* root) {
  DirTreeModelItem* item = new DirTreeModelItem();
  int row = rootItems_.count();
  beginInsertRows(QModelIndex(), row, row);
  item->fileInfo_ = fm_file_info_ref(root);
  rootItems_.append(item);
  // add_place_holder_child_item(model, item_l, NULL, FALSE);
  endInsertRows();
  return QModelIndex();
}

DirTreeModelItem* DirTreeModel::itemFromIndex(const QModelIndex& index) {
  return NULL;
}

void DirTreeModel::loadRow(const QModelIndex& index) {
}

void DirTreeModel::unloadRow(const QModelIndex& index) {
}

bool DirTreeModel::isLoaded(const QModelIndex& index) {
  return false;
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
