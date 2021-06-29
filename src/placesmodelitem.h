/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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


#ifndef FM_PLACESMODELITEM_H
#define FM_PLACESMODELITEM_H

#include "libfmqtglobals.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QList>
#include <QAction>
#include <libfm/fm.h>

namespace Fm {

// model item
class LIBFM_QT_API PlacesModelItem : public QStandardItem {
public:
  enum Type {
    Places = QStandardItem::UserType + 1,
    Volume,
    Mount,
    Bookmark
  };

public:
  PlacesModelItem();
  PlacesModelItem(QIcon icon, QString title, FmPath* path = NULL);
  PlacesModelItem(const char* iconName, QString title, FmPath* path = NULL);
  PlacesModelItem(FmIcon* icon, QString title, FmPath* path = NULL);
  ~PlacesModelItem();

  FmFileInfo* fileInfo() {
    return fileInfo_;
  }
  void setFileInfo(FmFileInfo* fileInfo);

  FmPath* path() {
    return path_;
  }
  void setPath(FmPath* path);

  FmIcon* icon() {
    return icon_;
  }
  void setIcon(FmIcon* icon);
  void setIcon(GIcon* gicon);
  void updateIcon();

  QVariant data(int role = Qt::UserRole + 1) const;

  virtual int type() const {
    return Places;
  }

private:
  FmPath* path_;
  FmFileInfo* fileInfo_;
  FmIcon* icon_;
};

class LIBFM_QT_API PlacesModelVolumeItem : public PlacesModelItem {
public:
  PlacesModelVolumeItem(GVolume* volume);
  bool isMounted();
  bool canEject() {
    return g_volume_can_eject(volume_);
  }
  virtual int type() const {
    return Volume;
  }
  GVolume* volume() {
    return volume_;
  }
  void update();
private:
  GVolume* volume_;
};

class LIBFM_QT_API PlacesModelMountItem : public PlacesModelItem {
public:
  PlacesModelMountItem(GMount* mount);
  virtual int type() const {
    return Mount;
  }
  GMount* mount() const {
    return mount_;
  }
  void update();
private:
  GMount* mount_;
};

class LIBFM_QT_API PlacesModelBookmarkItem : public PlacesModelItem {
public:
  virtual int type() const {
    return Bookmark;
  }
  PlacesModelBookmarkItem(FmBookmarkItem* bm_item);
  virtual ~PlacesModelBookmarkItem() {
    if(bookmarkItem_)
      fm_bookmark_item_unref(bookmarkItem_);
  }
  FmBookmarkItem* bookmark() const {
    return bookmarkItem_;
  }
private:
  FmBookmarkItem* bookmarkItem_;
};

}

#endif // FM_PLACESMODELITEM_H
