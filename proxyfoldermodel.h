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


#ifndef FM_PROXYFOLDERMODEL_H
#define FM_PROXYFOLDERMODEL_H

#include <QSortFilterProxyModel>
#include <libfm/fm.h>

namespace Fm {

// a proxy model used to sort and filter FolderModel

class ProxyFolderModel : public QSortFilterProxyModel
{
Q_OBJECT

public:
  explicit ProxyFolderModel(QObject * parent = 0);
  ~ProxyFolderModel();

  void setShowHidden(bool show);
  bool showHidden() {
    return showHidden_;
  }

  FmFileInfo* fileInfoFromIndex(const QModelIndex& index) const;

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;
  bool lessThan ( const QModelIndex & left, const QModelIndex & right ) const;

private:
  bool showHidden_;
};

}

#endif // FM_PROXYFOLDERMODEL_H
