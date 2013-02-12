/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

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

#include <QTreeView>
#include <libfm/fm.h>

namespace Fm {
  
class PlacesModel;

class PlacesView : public QTreeView
{
Q_OBJECT

public:
  explicit PlacesView(QWidget* parent = 0);
  virtual ~PlacesView();

  void chdir(FmPath* path);

protected Q_SLOTS:
  void clicked(const QModelIndex & index);
  void dragMoveEvent(QDragMoveEvent* event);
  void dropEvent(QDropEvent* event);
  void contextMenuEvent(QContextMenuEvent* event);

protected:
  void drawBranches ( QPainter * painter, const QRect & rect, const QModelIndex & index ) const {
    // override this method to inhibit drawing of the branch grid lines by Qt.
  }

private:
  PlacesModel* model_;
};

}

#endif // FM_PLACESVIEW_H
