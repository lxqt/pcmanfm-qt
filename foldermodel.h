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


#ifndef FM_FOLDERMODEL_H
#define FM_FOLDERMODEL_H

#include <QAbstractListModel>
#include <QIcon>
#include <libfm/fm.h>
#include <QList>

namespace Fm {

class FolderModel : public QAbstractListModel
{
public:

  enum Role {
    FileInfoRole = Qt::UserRole
  };

  enum ColumnId {
    ColumnName,
    ColumnFileType,
    ColumnMTime,
    NumOfColumns
  };
  
  class Item {
  public:
    Item(FmFileInfo* _info):
      info(fm_file_info_ref(_info)) {
      displayName = QString::fromUtf8(fm_file_info_get_disp_name(info));
    }

    Item(const Item& other) {
      info = other.info ? fm_file_info_ref(other.info) : NULL;
      displayName = QString::fromUtf8(fm_file_info_get_disp_name(info));
    }

    ~Item() {
      if(info)
	fm_file_info_unref(info);
    }

    QString displayName;
    FmFileInfo* info;
  };

public:
  FolderModel();
  virtual ~FolderModel();

  void setFolder(FmFolder* new_folder);

  int rowCount(const QModelIndex & parent) const;
  int columnCount (const QModelIndex & parent) const;
  QVariant data(const QModelIndex & index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;
  QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

protected:
  static void onStartLoading(FmFolder* folder, gpointer user_data);
  static void onFinishLoading(FmFolder* folder, gpointer user_data);
  static void onFilesAdded(FmFolder* folder, GSList* files, gpointer user_data);
  static void onFilesChanged(FmFolder* folder, GSList* files, gpointer user_data);
  static void onFilesRemoved(FmFolder* folder, GSList* files, gpointer user_data);
  void insertFiles(int row, FmFileInfoList* files);
  void removeAll();
  QList<Item>::iterator findItemByPath(FmPath* path, int* row);
  QList<Item>::iterator findItemByName(const char* name, int* row);

private:
  FmFolder* folder;
  QList<Item> items;
};

}

#endif // FM_FOLDERMODEL_H
