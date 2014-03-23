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


#ifndef FM_FOLDERVIEW_H
#define FM_FOLDERVIEW_H

#include "libfmqtglobals.h"
#include <QWidget>
#include <QListView>
#include <QTreeView>
#include <QMouseEvent>
#include <libfm/fm.h>
#include "foldermodel.h"
#include "proxyfoldermodel.h"

class QTimer;

namespace Fm {

class FileMenu;
class FolderMenu;
class FileLauncher;

// override these classes for implementing FolderView
class FolderViewListView : public QListView {
public:
  FolderViewListView(QWidget* parent = 0);
  virtual ~FolderViewListView();
  virtual void startDrag(Qt::DropActions supportedActions);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dragMoveEvent(QDragMoveEvent* e);
  virtual void dragLeaveEvent(QDragLeaveEvent* e);
  virtual void dropEvent(QDropEvent* e);
  
  virtual QModelIndex indexAt(const QPoint & point) const;
};

class FolderViewTreeView : public QTreeView {
  Q_OBJECT
public:
  FolderViewTreeView(QWidget* parent = 0);
  virtual ~FolderViewTreeView();
  virtual void setModel(QAbstractItemModel* model);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dragMoveEvent(QDragMoveEvent* e);
  virtual void dragLeaveEvent(QDragLeaveEvent* e);
  virtual void dropEvent(QDropEvent* e);
  
  virtual void rowsInserted(const QModelIndex& parent,int start, int end);
  virtual void rowsAboutToBeRemoved(const QModelIndex& parent,int start, int end);
  virtual void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

  virtual void resizeEvent(QResizeEvent* event);
  void queueLayoutColumns();

private Q_SLOTS:
  void layoutColumns();

private:
  bool doingLayout_;
  QTimer* layoutTimer_;
};

class LIBFM_QT_API FolderView : public QWidget {
  Q_OBJECT

public:
  enum ViewMode {
    FirstViewMode = 1,
    IconMode = FirstViewMode,
    CompactMode,
    DetailedListMode,
    ThumbnailMode,
    LastViewMode = ThumbnailMode,
    NumViewModes = (LastViewMode - FirstViewMode + 1)
  };

  enum ClickType {
    ActivatedClick,
    MiddleClick,
    ContextMenuClick
  };

public:

  friend class FolderViewTreeView;
  friend class FolderViewListView;
  
  explicit FolderView(ViewMode _mode = IconMode, QWidget* parent = 0);
  virtual ~FolderView();

  void setViewMode(ViewMode _mode);
  ViewMode viewMode() const;
  
  void setIconSize(ViewMode mode, QSize size);
  QSize iconSize(ViewMode mode) const;

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

  void setFileLauncher(FileLauncher* launcher) {
    fileLauncher_ = launcher;
  }

  FileLauncher* fileLauncher() {
    return fileLauncher_;
  }

protected:
  void contextMenuEvent(QContextMenuEvent* event);
  void childMousePressEvent(QMouseEvent* event);
  void childDragEnterEvent(QDragEnterEvent* event);
  void childDragMoveEvent(QDragMoveEvent* e);
  void childDragLeaveEvent(QDragLeaveEvent* e);
  void childDropEvent(QDropEvent* e);

  void emitClickedAt(ClickType type, QPoint& pos);

  QModelIndexList selectedRows ( int column = 0 ) const;
  QModelIndexList selectedIndexes() const;

  virtual void prepareFileMenu(Fm::FileMenu* menu);
  virtual void prepareFolderMenu(Fm::FolderMenu* menu);

public Q_SLOTS:
  void onItemActivated(QModelIndex index);
  void onSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
  virtual void onFileClicked(int type, FmFileInfo* fileInfo);
  
Q_SIGNALS:
  void clicked(int type, FmFileInfo* file);
  void selChanged(int n_sel);
  void sortChanged();

private:

  QAbstractItemView* view;
  ProxyFolderModel* model_;
  ViewMode mode;
  QSize iconSize_[NumViewModes];
  FileLauncher* fileLauncher_;
};

}

#endif // FM_FOLDERVIEW_H
