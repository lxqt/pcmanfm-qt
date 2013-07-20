/*

    Copyright (C) 2012  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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


#ifndef FM_PLACESVIEW_H
#define FM_PLACESVIEW_H

#include "libfmqtglobals.h"
#include <QTreeView>
#include <libfm/fm.h>
#include "placesmodel.h"

namespace Fm {
  
class LIBFM_QT_API PlacesView : public QTreeView {
Q_OBJECT

public:
  explicit PlacesView(QWidget* parent = 0);
  virtual ~PlacesView();

  void setCurrentPath(FmPath* path);
  FmPath* currentPath() {
    return currentPath_;
  }

Q_SIGNALS:
  void chdirRequested(int type, FmPath* path);

protected Q_SLOTS:
  void onClicked(const QModelIndex & index);
  // void onMountOperationFinished(GError* error);

  void onEmptyTrash();

  void onMountVolume();
  void onUnmountVolume();
  void onEjectVolume();
  void onUnmountMount();

  void onDeleteBookmark();
  void onRenameBookmark();

protected:
  void drawBranches ( QPainter * painter, const QRect & rect, const QModelIndex & index ) const {
    // override this method to inhibit drawing of the branch grid lines by Qt.
  }

  virtual void dragMoveEvent(QDragMoveEvent* event);
  virtual void dropEvent(QDropEvent* event);
  virtual void contextMenuEvent(QContextMenuEvent* event);

private:
  PlacesModel* model_;
  FmPath* currentPath_;
};

}

#endif // FM_PLACESVIEW_H
