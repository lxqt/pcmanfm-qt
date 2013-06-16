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


#include "proxyfoldermodel.h"
#include "foldermodel.h"

using namespace Fm;

ProxyFolderModel::ProxyFolderModel(QObject * parent):
  QSortFilterProxyModel(parent),
  thumbnailSize_(0),
  showHidden_(false),
  showThumbnails_(false),
  folderFirst_(true) {
  setDynamicSortFilter(true);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

ProxyFolderModel::~ProxyFolderModel() {
  qDebug("delete ProxyFolderModel");

  if(showThumbnails_ && thumbnailSize_ != 0) {
    FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
    // tell the source model that we don't need the thumnails anymore
    if(srcModel) {
      srcModel->releaseThumbnails(thumbnailSize_);
      disconnect(srcModel, SIGNAL(thumbnailLoaded(QModelIndex,int)));
    }
  }
}

void ProxyFolderModel::setSourceModel(QAbstractItemModel* model) {
  // we only support Fm::FolderModel
  Q_ASSERT(model->inherits("Fm::FolderModel"));

  if(showThumbnails_ && thumbnailSize_ != 0) { // if we're showing thumbnails
    FolderModel* oldSrcModel = static_cast<FolderModel*>(sourceModel());
    FolderModel* newSrcModel = static_cast<FolderModel*>(model);
    if(oldSrcModel) { // we need to release cached thumbnails for the old source model
      oldSrcModel->releaseThumbnails(thumbnailSize_);
      disconnect(oldSrcModel, SIGNAL(thumbnailLoaded(QModelIndex,int)));
    }
    if(newSrcModel) { // tell the new source model that we want thumbnails of this size
      newSrcModel->cacheThumbnails(thumbnailSize_);
      connect(newSrcModel, SIGNAL(thumbnailLoaded(QModelIndex,int)), SLOT(onThumbnailLoaded(QModelIndex,int)));
    }
  }
  QSortFilterProxyModel::setSourceModel(model);
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
    QAbstractItemModel* srcModel = sourceModel();
    QString name = srcModel->data(srcModel->index(source_row, 0, source_parent)).toString();
    if(name.startsWith(".") || name.endsWith("~"))
      return false;
  }
  // apply additional filters if there're any
  Q_FOREACH(ProxyFolderModelFilter* filter, filters_) {
    FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
    FmFileInfo* fileInfo = srcModel->fileInfoFromIndex(srcModel->index(source_row, 0, source_parent));
    if(!filter->filterAcceptsRow(this, fileInfo))
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

void ProxyFolderModel::setShowThumbnails(bool show) {
  if(show != showThumbnails_) {
    showThumbnails_ = show;
    FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
    if(srcModel && thumbnailSize_ != 0) {
      if(show) {
        // ask for cache of thumbnails of the new size in source model
        srcModel->cacheThumbnails(thumbnailSize_);
        // connect to the srcModel so we can be notified when a thumbnail is loaded.
        connect(srcModel, SIGNAL(thumbnailLoaded(QModelIndex,int)), SLOT(onThumbnailLoaded(QModelIndex,int)));
      }
      else { // turn off thumbnails
        // free cached old thumbnails in souce model
        srcModel->releaseThumbnails(thumbnailSize_);
        disconnect(srcModel, SIGNAL(thumbnailLoaded(QModelIndex,int)));
      }
      // reload all items, FIXME: can we only update items previously having thumbnails
      Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
    }
  }
}

void ProxyFolderModel::setThumbnailSize(int size) {
  if(size != thumbnailSize_) {
    FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
    if(showThumbnails_ && srcModel) {
      // free cached thumbnails of the old size
      if(thumbnailSize_ != 0)
        srcModel->releaseThumbnails(thumbnailSize_);
      else {
        // if the old thumbnail size is 0, we did not turn on thumbnail initially
        connect(srcModel, SIGNAL(thumbnailLoaded(QModelIndex,int)), SLOT(onThumbnailLoaded(QModelIndex,int)));
      }
      // ask for cache of thumbnails of the new size in source model
      srcModel->cacheThumbnails(size);
      // reload all items, FIXME: can we only update items previously having thumbnails
      Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
    }
    thumbnailSize_ = size;
  }
}

QVariant ProxyFolderModel::data(const QModelIndex& index, int role) const {
  if(role == Qt::DecorationRole && showThumbnails_ && thumbnailSize_) {
    // we need to show thumbnails instead of icons
    FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
    QModelIndex srcIndex = mapToSource(index);
    QImage image = srcModel->thumbnailFromIndex(srcIndex, thumbnailSize_);
    if(!image.isNull()) // if we got a thumbnail of the desired size, use it
      return QVariant(image);
  }
  // fallback to icons if thumbnails are not available
  return QSortFilterProxyModel::data(index, role);
}

void ProxyFolderModel::onThumbnailLoaded(const QModelIndex& srcIndex, int size) {
  
  FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
  FolderModelItem* item = srcModel->itemFromIndex(srcIndex);
  // qDebug("ProxyFolderModel::onThumbnailLoaded: %d, %s", size, item->displayName.toUtf8().data());
  
  if(size == thumbnailSize_) { // if a thumbnail of the size we want is loaded
    QModelIndex index = mapFromSource(srcIndex);
    Q_EMIT dataChanged(index, index);
  }
}

void ProxyFolderModel::addFilter(ProxyFolderModelFilter* filter) {
  filters_.append(filter);
  invalidateFilter();
  Q_EMIT sortFilterChanged();
}

void ProxyFolderModel::removeFilter(ProxyFolderModelFilter* filter) {
  filters_.removeOne(filter);
  invalidateFilter();
  Q_EMIT sortFilterChanged();  
}


#if 0
void ProxyFolderModel::reloadAllThumbnails() {
  // reload all thumbnails and update UI
  FolderModel* srcModel = static_cast<FolderModel*>(sourceModel());
  if(srcModel) {
    int rows= rowCount();
    for(int row = 0; row < rows; ++row) {
      QModelIndex index = this->index(row, 0);
      QModelIndex srcIndex = mapToSource(index);
      QImage image = srcModel->thumbnailFromIndex(srcIndex, size);
      // tell the world that the item is changed to trigger a UI update
      if(!image.isNull())
        Q_EMIT dataChanged(index, index);
    }
  }
}
#endif
