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


#ifndef FM_FOLDERVIEW_H
#define FM_FOLDERVIEW_H

#include <QWidget>
#include <QListView>
#include <QTreeView>
#include <QMouseEvent>
#include <libfm/fm.h>
#include "foldermodel.h"
#include "proxyfoldermodel.h"

namespace Fm {

class FileMenu;

class FolderView : public QWidget
{
  Q_OBJECT

public:
  enum ViewMode {
    IconMode = 1,
    CompactMode,
    DetailedListMode,
    ThumbnailMode
  };

  enum ClickType {
    ActivatedClick,
    MiddleClick,
    ContextMenuClick
  };

protected:

  // override these classes for implementing FolderView
  class ListView : public QListView {
  public:
    ListView(QWidget* parent = 0) : QListView(parent) {
    }
    void mousePressEvent(QMouseEvent* event);
  };

  class TreeView : public QTreeView {
  public:
    TreeView(QWidget* parent = 0) : QTreeView(parent) {
    }
    void mousePressEvent(QMouseEvent* event);
  };

public:
  explicit FolderView(ViewMode _mode = IconMode, QWidget* parent = 0);
  ~FolderView();

  void setViewMode(ViewMode _mode);
  ViewMode viewMode() const;
  
  void setIconSize(QSize size);
  QSize iconSize() const;

  void setGridSize(QSize size);
  QSize gridSize() const;

  QAbstractItemView* childView() const;

  ProxyFolderModel* model() const;
  void setModel(ProxyFolderModel* _model);
  
  FmFolder* folder() {
    return model_ ? static_cast<FolderModel*>(model_->sourceModel())->folder() : NULL;
  }

  FmFileInfo* folderInfo() {
    FmFolder* _folder = folder();
    return _folder ? fm_folder_get_info(_folder) : NULL;
  }

  FmPath* path() {
    FmFolder* _folder = folder();
    return _folder ? fm_folder_get_path(_folder) : NULL;
  }

  QItemSelectionModel* selectionModel() const;
  FmFileInfoList* selectedFiles() const;
  FmPathList* selectedFilePaths() const;

  void selectAll() {
    view->selectAll();
  }

  void invertSelection();
  
protected:
  void contextMenuEvent(QContextMenuEvent* event);
  void childMousePressEvent(QMouseEvent* event);
  void emitClickedAt(ClickType type, QPoint& pos);

  QModelIndexList selectedRows ( int column = 0 ) const;
  QModelIndexList selectedIndexes() const;

public Q_SLOTS:
  void onItemActivated(QModelIndex index);

Q_SIGNALS:
  void clicked(int type, FmFileInfo* file);
  void selChanged(int n_sel);
  void sortChanged();

private:

  QAbstractItemView* view;
  ProxyFolderModel* model_;
  ViewMode mode;
  QSize iconSize_;
  QSize gridSize_;
};

}

#endif // FM_FOLDERVIEW_H
