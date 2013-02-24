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
#include <QAction>
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
      Item(QString title, FmPath* path = 0);
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
      
      virtual int type() const {
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
      bool canEject() {
        return g_volume_can_eject(volume_);
      }
      virtual int type() const {
        return ItemTypeVolume;
      }
      GVolume* volume() {
        return volume_;
      }
      void update();
    private:
      GVolume* volume_;
  };

  class MountItem : public Item {
    public:
      MountItem(GMount* mount);
      virtual int type() const {
        return ItemTypeMount;
      }
      GMount* mount() const {
        return mount_;
      }
      void update();
    private:
      GMount* mount_;
  };

  class BookmarkItem : public Item {
    public:
      virtual int type() const {
        return ItemTypeBookmark;
      }
      BookmarkItem(FmBookmarkItem* bm_item);
      virtual ~BookmarkItem() {
        if(bookmarkItem_)
          fm_bookmark_item_unref(bookmarkItem_);
      }
      FmBookmarkItem* bookmark() const {
        return bookmarkItem_;
      }
    private:
      FmBookmarkItem* bookmarkItem_;
  };

  // QAction used for popup menus
  class ItemAction : public QAction {
  public:
    ItemAction(Item* item, QString text, QObject* parent = 0):
      QAction(text, parent),
      item_(item) {
    }

    Item* item() {
      return item_;
    }
  private:
    Item* item_;
  };

public:
  explicit PlacesModel(QObject* parent = 0);
  virtual ~PlacesModel();

  bool showTrash() {
    return showTrash_;
  }
  void setShowTrash(bool show);

  bool showApplications() {
    return showApplications_;
  }
  void setShowApplications(bool show);

  bool showDesktop() {
    return showDesktop_;
  }
  void setShowDesktop(bool show);

  /*
  virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const;
  virtual Qt::DropActions supportedDropActions() const;
  */
protected:
  
  Item* itemFromPath(FmPath* path);
  Item* itemFromPath(QStandardItem* rootItem, FmPath* path);
  VolumeItem* itemFromVolume(GVolume* volume);
  MountItem* itemFromMount(GMount* mount);
  BookmarkItem* itemFromBookmark(FmBookmarkItem* bkitem);

private:
  void loadBookmarks();
  
  static void onVolumeAdded(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis);
  static void onVolumeRemoved(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis);
  static void onVolumeChanged(GVolumeMonitor* monitor, GVolume* volume, PlacesModel* pThis);
  static void onMountAdded(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis);
  static void onMountRemoved(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis);
  static void onMountChanged(GVolumeMonitor* monitor, GMount* mount, PlacesModel* pThis);
  
  static void onBookmarksChanged(FmBookmarks* bookmarks, PlacesModel* pThis);

private:
  FmBookmarks* bookmarks;
  GVolumeMonitor* volumeMonitor;
  QList<FmJob*> jobs;
  bool showTrash_;
  bool showApplications_;
  bool showDesktop_;
  QStandardItem* placesRoot;
  QStandardItem* devicesRoot;
  QStandardItem* bookmarksRoot;
  Item* trashItem;
  Item* desktopItem;
  Item* homeItem;
  Item* computerItem;
  Item* networkItem;
  Item* applicationsItem;
};

}

#endif // FM_PLACESMODEL_H
