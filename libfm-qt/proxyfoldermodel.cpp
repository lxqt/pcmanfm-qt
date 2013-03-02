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


#include "proxyfoldermodel.h"
#include "foldermodel.h"

using namespace Fm;

ProxyFolderModel::ProxyFolderModel(QObject * parent):
  QSortFilterProxyModel(parent),
  showHidden_(false),
  folderFirst_(true) {
  setDynamicSortFilter(true);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

ProxyFolderModel::~ProxyFolderModel() {
}

void ProxyFolderModel::sort(int column, Qt::SortOrder order) {
  int oldColumn = sortColumn();
  Qt::SortOrder oldOrder = sortOrder();
  QSortFilterProxyModel::sort(column, order);
  if(column != oldColumn || order != oldOrder) {
    Q_EMIT sortFilterChanged();
  }
}

void ProxyFolderModel::setShowHidden(bool show) {
  if(show != showHidden_) {
    showHidden_ = show;
    invalidateFilter();
    Q_EMIT sortFilterChanged();
  }
}

// need to call invalidateFilter() manually.
void ProxyFolderModel::setFolderFirst(bool folderFirst) {
  if(folderFirst != folderFirst_) {
    folderFirst_ = folderFirst;
    invalidate();
    Q_EMIT sortFilterChanged();
  }
}

bool ProxyFolderModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const {
  if(!showHidden_) {
    QAbstractItemModel* model = sourceModel();
    QString name = model->data(model->index(source_row, 0, source_parent)).toString();
    if(name.startsWith(".") || name.endsWith("~"))
      return false;
  }
  return true;
}

bool ProxyFolderModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
  FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
  // left and right are indexes of source model, not the proxy model.
  if(srcModel) {
    FmFileInfo* leftInfo = srcModel->fileInfoFromIndex(left);
    FmFileInfo* rightInfo = srcModel->fileInfoFromIndex(right);
    
    if(folderFirst_) {
      bool leftIsFolder = (bool)fm_file_info_is_dir(leftInfo);
      bool rightIsFolder = (bool)fm_file_info_is_dir(rightInfo);
      if(leftIsFolder != rightIsFolder)
        return leftIsFolder ? true : false;
    }

    // FIXME: do we need to handle filterCaseSensitivity() here?

    switch(sortColumn()) {
      case FolderModel::ColumnFileName:
        break;
      case FolderModel::ColumnFileMTime:
        return fm_file_info_get_mtime(leftInfo) < fm_file_info_get_mtime(rightInfo);
      case FolderModel::ColumnFileSize:
        return fm_file_info_get_size(leftInfo) < fm_file_info_get_size(rightInfo);
      case FolderModel::ColumnFileOwner:
        // TODO: sort by owner
        break;
      case FolderModel::ColumnFileType:
        break;
    }
  }
  return QSortFilterProxyModel::lessThan(left, right);
}

FmFileInfo* ProxyFolderModel::fileInfoFromIndex(const QModelIndex& index) const {
  FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
  if(srcModel) {
    QModelIndex srcIndex = mapToSource(index);
    return srcModel->fileInfoFromIndex(srcIndex);
  }
  return NULL;
}

#include "proxyfoldermodel.moc"
