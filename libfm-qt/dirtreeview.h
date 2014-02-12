/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef FM_DIRTREEVIEW_H
#define FM_DIRTREEVIEW_H

#include "libfmqtglobals.h"
#include <QTreeView>
#include <libfm/fm.h>
#include "dirtreemodel.h"

namespace Fm {

class LIBFM_QT_API DirTreeView : public QTreeView {
  Q_OBJECT

public:
  DirTreeView(QWidget* parent);
  ~DirTreeView();

  FmPath* currentPath() {
    return currentPath_;
  }

  void setCurrentPath(FmPath* path);

  // libfm-gtk compatible alias
  FmPath* getCwd() {
    return currentPath();
  }

  void chdir(FmPath* path) {
    setCurrentPath(path);
  }

  virtual void setModel(QAbstractItemModel* model);

protected:
  virtual void contextMenuEvent(QContextMenuEvent* event);

Q_SIGNALS:
  void chdirRequested(int type, FmPath* path);

protected Q_SLOTS:
  void onCollapsed(const QModelIndex & index);
  void onExpanded(const QModelIndex & index);
  void onActivated(const QModelIndex & index);
  void onClicked(const QModelIndex & index);

  void onCurrentRowChanged(const QModelIndex & current, const QModelIndex & previous);

private:
  FmPath* currentPath_;
};

}

#endif // FM_DIRTREEVIEW_H
