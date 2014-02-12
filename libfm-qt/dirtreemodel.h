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

#ifndef FM_DIRTREEMODEL_H
#define FM_DIRTREEMODEL_H

#include "libfmqtglobals.h"
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QIcon>
#include <QList>
#include <libfm/fm.h>

namespace Fm {
  
class DirTreeModelItem;

class LIBFM_QT_API DirTreeModel : public QAbstractItemModel {
  Q_OBJECT

public:
  DirTreeModel(QObject* parent);
  ~DirTreeModel();

  QModelIndex addRoot(FmFileInfo* root);
  void loadRow(const QModelIndex& index);
  void unloadRow(const QModelIndex& index);

  bool isLoaded(const QModelIndex& index);
  QIcon icon(const QModelIndex& index);
  FmFileInfo* fileInfo(const QModelIndex& index);
  FmPath* filePath(const QModelIndex& index);
  const char* dispName(const QModelIndex& index);

  DirTreeModelItem* itemFromIndex(const QModelIndex& index);

  void setShowHidden(bool show_hidden);
  bool showHidden() const {
    return showHidden_;
  }
  
  virtual QVariant data(const QModelIndex& index, int role);
  virtual int columnCount(const QModelIndex& parent);
  virtual int rowCount(const QModelIndex& parent);
  virtual QModelIndex parent(const QModelIndex& child);
  virtual QModelIndex index(int row, int column, const QModelIndex& parent);

private:
  bool showHidden_;
  QList<DirTreeModelItem*> rootItems_;
};
}

#endif // FM_DIRTREEMODEL_H
