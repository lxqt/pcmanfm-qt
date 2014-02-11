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


#ifndef FM_PLACESMODEL_H
#define FM_PLACESMODEL_H

#include "libfmqtglobals.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QList>
#include <QAction>
#include <libfm/fm.h>
#include "placesmodelitem.h"

namespace Fm {

class LIBFM_QT_API PlacesModel : public QStandardItemModel {
Q_OBJECT
friend class PlacesView;
public:

  // QAction used for popup menus
  class ItemAction : public QAction {
  public:
    ItemAction(PlacesModelItem* item, QString text, QObject* parent = 0):
      QAction(text, parent),
      item_(item) {
    }

    PlacesModelItem* item() {
      return item_;
    }
  private:
    PlacesModelItem* item_;
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

public Q_SLOTS:
  void updateIcons();

protected:
  
  PlacesModelItem* itemFromPath(FmPath* path);
  PlacesModelItem* itemFromPath(QStandardItem* rootItem, FmPath* path);
  PlacesModelVolumeItem* itemFromVolume(GVolume* volume);
  PlacesModelMountItem* itemFromMount(GMount* mount);
  PlacesModelBookmarkItem* itemFromBookmark(FmBookmarkItem* bkitem);

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
  PlacesModelItem* trashItem;
  PlacesModelItem* desktopItem;
  PlacesModelItem* homeItem;
  PlacesModelItem* computerItem;
  PlacesModelItem* networkItem;
  PlacesModelItem* applicationsItem;
  QIcon ejectIcon_;
};

}

#endif // FM_PLACESMODEL_H
