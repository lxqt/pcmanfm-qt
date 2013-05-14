/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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
#include <QList>

namespace Fm {

// a proxy model used to sort and filter FolderModel

class FolderModelItem;
class ProxyFolderModel;

class LIBFM_QT_API ProxyFolderModelFilter {
public:
  virtual bool filterAcceptsRow(const ProxyFolderModel* model, FmFileInfo* info) const = 0;
  virtual ~ProxyFolderModelFilter();
};


class LIBFM_QT_API ProxyFolderModel : public QSortFilterProxyModel {
  Q_OBJECT
public:

public:
  explicit ProxyFolderModel(QObject * parent = 0);
  virtual ~ProxyFolderModel();

  // only Fm::FolderModel is allowed for being sourceModel
  virtual void setSourceModel(QAbstractItemModel* model);

  void setShowHidden(bool show);
  bool showHidden() {
    return showHidden_;
  }

  void setFolderFirst(bool folderFirst);
  bool folderFirst() {
    return folderFirst_;
  }

  void setSortCaseSensitivity(Qt::CaseSensitivity cs) {
    QSortFilterProxyModel::setSortCaseSensitivity(cs);
    Q_EMIT sortFilterChanged();
  }

  bool showThumbnails() {
    return showThumbnails_;
  }
  void setShowThumbnails(bool show);

  int thumbnailSize() {
    return thumbnailSize_;
  }
  void setThumbnailSize(int size);

  FmFileInfo* fileInfoFromIndex(const QModelIndex& index) const;

  virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

  void addFilter(ProxyFolderModelFilter* filter);
  void removeFilter(ProxyFolderModelFilter* filter);

Q_SIGNALS:
  void sortFilterChanged();

protected Q_SLOTS:
  void onThumbnailLoaded(const QModelIndex& srcIndex, int size);

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;
  bool lessThan(const QModelIndex & left, const QModelIndex & right) const;
  // void reloadAllThumbnails();

private:

private:
  bool showHidden_;
  bool folderFirst_;
  bool showThumbnails_;
  int thumbnailSize_;
  QList<ProxyFolderModelFilter*> filters_;
};

}

#endif // FM_PROXYFOLDERMODEL_H
