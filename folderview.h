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

namespace Fm {

class ProxyFolderModel;
  
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
  // override these classes just to handle mouse events
  class ListView : public QListView {
  public:
    ListView(QWidget* parent = 0) : QListView(parent) {
    }
    void mousePressEvent(QMouseEvent* event) {
      QListView::mousePressEvent(event);
      if(event->button() == Qt::MiddleButton) {
	QPoint pos = event->pos();
	static_cast<FolderView*>(parent())->emitClickedAt(MiddleClick, pos);
      }
    }
  };

  class TreeView : public QTreeView {
  public:
    TreeView(QWidget* parent = 0) : QTreeView(parent) {
    }
    void mousePressEvent(QMouseEvent* event) {
      QTreeView::mousePressEvent(event);
      if(event->button() == Qt::MiddleButton) {
	QPoint pos = event->pos();
	static_cast<FolderView*>(parent())->emitClickedAt(MiddleClick, pos);
      }
    }
  };

public:
  explicit FolderView(ViewMode _mode = IconMode, QWidget* parent = 0);
  ~FolderView();

  void setViewMode(ViewMode _mode);
  ViewMode viewMode();
  
  void setIconSize(QSize size);
  QSize iconSize();

  void setGridSize(QSize size);
  QSize gridSize();

  QAbstractItemView* childView();

  ProxyFolderModel* model();
  void setModel(ProxyFolderModel* _model);

protected:
  void contextMenuEvent(QContextMenuEvent* event);
  void emitClickedAt(ClickType type, QPoint& pos);

public Q_SLOTS:
  void onItemActivated(QModelIndex index);

public:
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
