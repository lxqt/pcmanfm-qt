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
class FolderViewStyle;

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

  void selectAll();

  void invertSelection();

  void setFileLauncher(FileLauncher* launcher) {
    fileLauncher_ = launcher;
  }

  FileLauncher* fileLauncher() {
    return fileLauncher_;
  }

  int autoSelectionDelay() const {
    return autoSelectionDelay_;
  }

  void setAutoSelectionDelay(int delay);

protected:
  virtual bool event(QEvent* event);
  virtual void contextMenuEvent(QContextMenuEvent* event);
  virtual void childMousePressEvent(QMouseEvent* event);
  virtual void childDragEnterEvent(QDragEnterEvent* event);
  virtual void childDragMoveEvent(QDragMoveEvent* e);
  virtual void childDragLeaveEvent(QDragLeaveEvent* e);
  virtual void childDropEvent(QDropEvent* e);

  void emitClickedAt(ClickType type, const QPoint& pos);

  QModelIndexList selectedRows ( int column = 0 ) const;
  QModelIndexList selectedIndexes() const;

  virtual void prepareFileMenu(Fm::FileMenu* menu);
  virtual void prepareFolderMenu(Fm::FolderMenu* menu);
  
  virtual bool eventFilter(QObject* watched, QEvent* event);

  void updateGridSize(); // called when view mode, icon size, or font size is changed

public Q_SLOTS:
  void onItemActivated(QModelIndex index);
  void onSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
  virtual void onFileClicked(int type, FmFileInfo* fileInfo);

private Q_SLOTS:
  void onAutoSelectionTimeout();
  void onSelChangedTimeout();

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
  int autoSelectionDelay_;
  QTimer* autoSelectionTimer_;
  QModelIndex lastAutoSelectionIndex_;
  QTimer* selChangedTimer_;
};

}

#endif // FM_FOLDERVIEW_H
