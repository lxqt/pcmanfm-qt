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
public:
  
  enum ItemType {
    ItemTypePlaces = QStandardItem::UserType + 1,
    ItemTypeVolume,
    ItemTypeMount,
    ItemTypeBookmark
  };

  // model item
  class Item : public QStandardItem {
    public:
      Item(FmPath* path = 0);
      Item(QIcon icon, QString title, FmPath* path = 0);
      Item(const char* iconName, QString title, FmPath* path = 0);
      Item(FmIcon* icon, QString title, FmPath* path = 0);
      Item(GIcon* gicon, QString title, FmPath* path = 0);
      ~Item();

      FmFileInfo* fileInfo() {
	return fileInfo_;
      }
      void setFileInfo(FmFileInfo* fileInfo);

      FmPath* path() {
	return path_;
      }
      void setPath(FmPath* path);

      QVariant data ( int role = Qt::UserRole + 1 ) const;
      
      int type() {
	return ItemTypePlaces;
      }

    private:
      FmPath* path_;
      FmFileInfo* fileInfo_;
  };

  class VolumeItem : public Item {
    public:
      VolumeItem(GVolume* volume);
      bool isMounted();
      int type() {
	return ItemTypeVolume;
      }
    private:
      GVolume* volume_;
  };

  class MountItem : public Item {
    public:
      MountItem(GMount* mount);
      bool isMounted();
      int type() {
	return ItemTypeMount;
      }
    private:
      GMount* mount_;
  };

  class BookmarkItem : public Item {
    public:
      int type() {
	return ItemTypeBookmark;
      }
    BookmarkItem(FmBookmarkItem* item) {
      bookmarkItem_ = fm_bookmark_item_ref(item);
    }
    virtual ~BookmarkItem() {
      if(bookmarkItem_)
	fm_bookmark_item_unref(bookmarkItem_);
    }
    private:
      FmBookmarkItem* bookmarkItem_;
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
