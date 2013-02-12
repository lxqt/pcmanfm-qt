/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef FM_PLACESMODEL_H
#define FM_PLACESMODEL_H

#include <QStandardItemModel>
#include <QStandardItem>
#include <QList>
#include <libfm/fm.h>

namespace Fm {

class PlacesModel : public QStandardItemModel
{
Q_OBJECT

protected:

  // model item
  class Item : public QStandardItem {
    public:
      Item();
      Item(QIcon icon, QString title);
      Item(const char* iconName, QString title);
      Item(FmIcon* icon, QString title);
      Item(GIcon* gicon, QString title);
      ~Item();

      void setIcon(FmIcon* icon);

      FmFileInfo* fileInfo() {
	return fileInfo_;
      }
      
      FmPath* path() {
	return fm_file_info_get_path(fileInfo_);
      }

      QVariant data ( int role = Qt::UserRole + 1 ) const;

    private:
      FmIcon* icon_;
      FmFileInfo* fileInfo_;
  };

public:
  explicit PlacesModel(QObject* parent = 0);
  virtual ~PlacesModel();
/*
  virtual QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const;
  virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const;
  virtual Qt::DropActions supportedDropActions() const;
  */

private:
  FmBookmarks* bookmarks;
  GVolumeMonitor* volumeMonitor;
  QList<FmJob*> jobs;
};

}

#endif // FM_PLACESMODEL_H
