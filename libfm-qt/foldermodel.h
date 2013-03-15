/*

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


#ifndef FM_FOLDERMODEL_H
#define FM_FOLDERMODEL_H

#include <QAbstractListModel>
#include <QIcon>
#include <QImage>
#include <libfm/fm.h>
#include <QList>
#include <QVector>
#include <QLinkedList>
#include <QPair>
#include "icontheme.h"
#include "foldermodelitem.h"

namespace Fm {

class LIBFM_QT_API FolderModel : public QAbstractListModel {
Q_OBJECT
public:

  enum Role {
    FileInfoRole = Qt::UserRole
  };

  enum ColumnId {
    ColumnFileName,
    ColumnFileType,
    ColumnFileSize,
    ColumnFileMTime,
    ColumnFileOwner,
    NumOfColumns
  };

public:
  FolderModel();
  virtual ~FolderModel();

  FmFolder* folder() {
    return folder_;
  }
  void setFolder(FmFolder* new_folder);

  FmPath* path() {
    return folder_ ? fm_folder_get_path(folder_) : NULL;
  }

  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  int columnCount (const QModelIndex & parent) const;
  QVariant data(const QModelIndex & index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
  QModelIndex parent( const QModelIndex & index ) const;
  // void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

  Qt::ItemFlags flags(const QModelIndex & index) const;

  virtual QStringList mimeTypes() const;
  virtual QMimeData* mimeData(const QModelIndexList & indexes) const;
  virtual Qt::DropActions supportedDropActions() const;
  virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);

  FmFileInfo* fileInfoFromIndex(const QModelIndex& index) const;
  FolderModelItem* itemFromIndex(const QModelIndex& index) const;
  QImage thumbnailFromIndex(const QModelIndex& index, int size);

  void cacheThumbnails(int size);
  void releaseThumbnails(int size);

Q_SIGNALS:
  void thumbnailLoaded(const QModelIndex& index, int size);

protected:
  static void onStartLoading(FmFolder* folder, gpointer user_data);
  static void onFinishLoading(FmFolder* folder, gpointer user_data);
  static void onFilesAdded(FmFolder* folder, GSList* files, gpointer user_data);
  static void onFilesChanged(FmFolder* folder, GSList* files, gpointer user_data);
  static void onFilesRemoved(FmFolder* folder, GSList* files, gpointer user_data);
  static void onThumbnailLoaded(FmThumbnailResult *res, gpointer user_data);

  void insertFiles(int row, FmFileInfoList* files);
  void removeAll();
  QList<FolderModelItem>::iterator findItemByPath(FmPath* path, int* row);
  QList<FolderModelItem>::iterator findItemByName(const char* name, int* row);
  QList<FolderModelItem>::iterator findItemByFileInfo(FmFileInfo* info, int* row);

private:
  FmFolder* folder_;
  // FIXME: should we use a hash table here so item lookup becomes much faster?
  QList<FolderModelItem> items;

  // record what size of thumbnails we should cache in an array of <size, refCount> pairs.
  QVector<QPair<int, int> > thumbnailRefCounts;
  QLinkedList<FmThumbnailResult*> thumbnailResults;
};

}

#endif // FM_FOLDERMODEL_H
