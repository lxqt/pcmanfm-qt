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


#include "foldermodel.h"
#include "icontheme.h"
#include <iostream>
#include <QtAlgorithms>
#include <QVector>
#include <qmimedata.h>
#include <QMimeData>
#include <QByteArray>
#include "utilities.h"
#include "fileoperation.h"

using namespace Fm;

FolderModel::FolderModel() : 
  folder_(NULL)  {
/*
    ColumnIcon,
    ColumnName,
    ColumnFileType,
    ColumnMTime,
    NumOfColumns
*/
  }

FolderModel::~FolderModel() {
  if(folder_)
    setFolder(NULL);
}

void FolderModel::setFolder(FmFolder* new_folder) {
  if(folder_) {
    removeAll();        // remove old items
    g_signal_handlers_disconnect_by_func(folder_, onStartLoading, this);
    g_signal_handlers_disconnect_by_func(folder_, onFinishLoading, this);
    g_signal_handlers_disconnect_by_func(folder_, onFilesAdded, this);
    g_signal_handlers_disconnect_by_func(folder_, onFilesChanged, this);
    g_signal_handlers_disconnect_by_func(folder_, onFilesRemoved, this);
    g_object_unref(folder_);
  }
  if(new_folder) {
    folder_ = FM_FOLDER(g_object_ref(new_folder));
    g_signal_connect(folder_, "start-loading", G_CALLBACK(onStartLoading), this);
    g_signal_connect(folder_, "finish-loading", G_CALLBACK(onFinishLoading), this);
    g_signal_connect(folder_, "files-added", G_CALLBACK(onFilesAdded), this);
    g_signal_connect(folder_, "files-changed", G_CALLBACK(onFilesChanged), this);
    g_signal_connect(folder_, "files-removed", G_CALLBACK(onFilesRemoved), this);
    // handle the case if the folder is already loaded
    if(fm_folder_is_loaded(folder_))
      insertFiles(0, fm_folder_get_files(folder_));
  }
  else
    folder_ = NULL;
}

void FolderModel::onStartLoading(FmFolder* folder, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
  // remove all items
  model->removeAll();
}

void FolderModel::onFinishLoading(FmFolder* folder, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
}

void FolderModel::onFilesAdded(FmFolder* folder, GSList* files, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
  int n_files = g_slist_length(files);
  model->beginInsertRows(QModelIndex(), model->items.count(), model->items.count() + n_files - 1);
  for(GSList* l = files; l; l = l->next) {
    FmFileInfo* info = FM_FILE_INFO(l->data);
    Item item(info);
/*
    if(fm_file_info_is_hidden(info)) {
      model->hiddenItems.append(item);
      continue;
    }
*/
    model->items.append(item);
  }
  model->endInsertRows();
}

static void FolderModel::onFilesChanged(FmFolder* folder, GSList* files, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
  int n_files = g_slist_length(files);
//  model->beginInsertRows(QModelIndex(), model->items.count(), model->items.count() + n_files - 1);
  for(GSList* l = files; l; l = l->next) {
    // Item item(FM_FILE_INFO(l->data));
  }
//  model->endInsertRows();
}

static void FolderModel::onFilesRemoved(FmFolder* folder, GSList* files, gpointer user_data) {
  FolderModel* model = static_cast<FolderModel*>(user_data);
  for(GSList* l = files; l; l = l->next) {
    FmFileInfo* info = FM_FILE_INFO(l->data);
    const char* name = fm_file_info_get_name(info);
    int row;
    QList<Item>::iterator it = model->findItemByName(name, &row);
    if(it != model->items.end()) {
      model->beginRemoveRows(QModelIndex(), row, row);
      model->items.erase(it);
      model->endRemoveRows();
    }
  }
}

void FolderModel::insertFiles(int row, FmFileInfoList* files) {
  int n_files = fm_file_info_list_get_length(files);
  beginInsertRows(QModelIndex(), row, row + n_files - 1);
  for(GList* l = fm_file_info_list_peek_head_link(files); l; l = l->next) {
    Item item(FM_FILE_INFO(l->data));
    items.append(item);
  }
  endInsertRows();
}

void FolderModel::removeAll() {
  if(items.empty())
    return;
  beginRemoveRows(QModelIndex(), 0, items.size() - 1);
  items.clear();
  endRemoveRows();
}

int FolderModel::rowCount(const QModelIndex & parent) const {
  if(parent.isValid())
    return 0;
  return items.size();
}

int FolderModel::columnCount (const QModelIndex & parent = QModelIndex()) const {
  if(parent.isValid())
    return 0;
  return NumOfColumns;
}

FolderModel::Item* FolderModel::itemFromIndex(const QModelIndex& index) const {
  return reinterpret_cast<Item*>(index.internalPointer());
}

FmFileInfo* FolderModel::fileInfoFromIndex(const QModelIndex& index) const {
  Item* item = itemFromIndex(index);
  return item ? item->info : NULL;
}

QVariant FolderModel::data(const QModelIndex & index, int role = Qt::DisplayRole) const {
  if(!index.isValid() || index.row() > items.size() || index.column() >= NumOfColumns) {
    return QVariant();
  }
  Item* item = itemFromIndex(index);
  FmFileInfo* info = item->info;

  switch(role) {
    case Qt::ToolTipRole:
      return QVariant(item->displayName);
    case Qt::DisplayRole:  {
      switch(index.column()) {
        case ColumnFileName: {
          return QVariant(item->displayName);
        }
        case ColumnFileType: {
          FmMimeType* mime = fm_file_info_get_mime_type(info);
          const char* desc = fm_mime_type_get_desc(mime);
          QString str = QString::fromUtf8(desc);
          return QVariant(str);
        }
        case ColumnFileMTime: {
          const char* name = fm_file_info_get_disp_mtime(info);
          QString str = QString::fromUtf8(name);
          return QVariant(str);
        }
        case ColumnFileSize: {
          const char* name = fm_file_info_get_disp_size(info);
          QString str = QString::fromUtf8(name);
          return QVariant(str);
          break;
        }
        case ColumnFileOwner: {
          // TODO: show owner names
          break;
        }
      }
    }
    case Qt::DecorationRole: {
      if(index.column() == 0) {
        // QPixmap pix = IconTheme::loadIcon(fm_file_info_get_icon(info), iconSize_);
        return QVariant(item->icon);
        // return QVariant(pix);
      }
    }
    case FileInfoRole:
      return qVariantFromValue((void*)info);
  }
  return QVariant();
}

QVariant FolderModel::headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const {
  if(role == Qt::DisplayRole) {
    if(orientation == Qt::Horizontal) {
      QString title;
      switch(section) {
        case ColumnFileName:
          title = tr("Name");
          break;
        case ColumnFileType:
          title = tr("Type");
          break;
        case ColumnFileSize:
          title = tr("Size");
          break;
        case ColumnFileMTime:
          title = tr("Modified");
          break;
        case ColumnFileOwner:
          title = tr("Owner");
          break;
      }
      return QVariant(title);
    }
  }
  return QVariant();
}

QModelIndex FolderModel::index(int row, int column, const QModelIndex & parent) const {
  if(row <0 || row >= items.size() || column < 0 || column >= NumOfColumns)
    return QModelIndex();
  const Item& item = items.at(row);
  return createIndex(row, column, (void*)&item);
}

QModelIndex FolderModel::parent(const QModelIndex & index) const {
  return QModelIndex();
}

Qt::ItemFlags FolderModel::flags(const QModelIndex& index) const {
  // FIXME: should not return same flags unconditionally for all columns
  Qt::ItemFlags flags;
  if(index.isValid()) {
    flags = Qt::ItemIsEnabled|Qt::ItemIsSelectable;
    if(index.column() == ColumnFileName)
      flags |= (Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled);
  }
  else {
    flags = Qt::ItemIsDropEnabled;
  }
  return flags;
}

// FIXME: this is very inefficient and should be replaced with a 
// more reasonable implementation later.
QList<FolderModel::Item>::iterator FolderModel::findItemByPath(FmPath* path, int* row) {
  QList<Item>::iterator it = items.begin();
  int i = 0;
  while(it != items.end()) {
    Item& item = *it;
    FmPath* item_path = fm_file_info_get_path(item.info);
    if(fm_path_equal(item_path, path)) {
      *row = i;
      return it;
    }
    ++it;
    ++i;
  }
  return items.end();
}

// FIXME: this is very inefficient and should be replaced with a 
// more reasonable implementation later.
QList<FolderModel::Item>::iterator FolderModel::findItemByName(const char* name, int* row) {
  QList<Item>::iterator it = items.begin();
  int i = 0;
  while(it != items.end()) {
    Item& item = *it;
    const char* item_name = fm_file_info_get_name(item.info);
    if(strcmp(name, item_name) == 0) {
      *row = i;
      return it;
    }
    ++it;
    ++i;
  }
  return items.end();
}

QStringList FolderModel::mimeTypes() const {
  qDebug("FolderModel::mimeTypes");
  QStringList types = QAbstractItemModel::mimeTypes();
  // now types contains "application/x-qabstractitemmodeldatalist"
  types << "text/uri-list";
  // types << "x-special/gnome-copied-files";
  return types;
}

QMimeData* FolderModel::mimeData(const QModelIndexList& indexes) const {
  QMimeData* data = QAbstractItemModel::mimeData(indexes);
  qDebug("FolderModel::mimeData");
  // build a uri list
  QByteArray urilist;
  urilist.reserve(4096);
  
  QModelIndexList::const_iterator it;
  for(it = indexes.constBegin(); it != indexes.end(); ++it) {
    const QModelIndex index = *it;
    Item* item = itemFromIndex(index);
    if(item) {
      FmPath* path = fm_file_info_get_path(item->info);
      char* uri = fm_path_to_uri(path);
      urilist.append(uri);
      urilist.append('\n');
      g_free(uri);
    }
  }
  data->setData("text/uri-list", urilist);
  
  return data;
}

bool FolderModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
  qDebug("FolderModel::dropMimeData");
  if(!folder_)
    return false;

  // FIXME: should we put this in dropEvent handler of FolderView instead?
  if(data->hasUrls()) {
    qDebug("drop action: %d", action);
    FmPathList* srcPaths = pathListFromQUrls(data->urls());
    switch(action) {
      case Qt::CopyAction:
        FileOperation::copyFiles(srcPaths, path());
        break;
      case Qt::MoveAction:
        FileOperation::moveFiles(srcPaths, path());
        break;
      case Qt::LinkAction:
        FileOperation::symlinkFiles(srcPaths, path());
      default:
        fm_path_list_unref(srcPaths);
        return false;
    }
    fm_path_list_unref(srcPaths);
    return true;
  }
  else if(data->hasFormat("application/x-qabstractitemmodeldatalist")) {
    return true;
  }
  return QAbstractListModel::dropMimeData(data, action, row, column, parent);
}

Qt::DropActions FolderModel::supportedDropActions() const {
  qDebug("FolderModel::supportedDropActions");
  return Qt::CopyAction|Qt::MoveAction|Qt::LinkAction;
}

