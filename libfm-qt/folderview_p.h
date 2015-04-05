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


#ifndef FM_FOLDERVIEW_P_H
#define FM_FOLDERVIEW_P_H

#include <QListView>
#include <QTreeView>
#include <QMouseEvent>
#include "folderview.h"

class QTimer;

namespace Fm {

// override these classes for implementing FolderView
class FolderViewListView : public QListView {
  Q_OBJECT
public:
  friend class FolderView;
  FolderViewListView(QWidget* parent = 0);
  virtual ~FolderViewListView();
  virtual void startDrag(Qt::DropActions supportedActions);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dragMoveEvent(QDragMoveEvent* e);
  virtual void dragLeaveEvent(QDragLeaveEvent* e);
  virtual void dropEvent(QDropEvent* e);
  
  virtual QModelIndex indexAt(const QPoint & point) const;

  inline void setPositionForIndex(const QPoint & position, const QModelIndex & index) {
    QListView::setPositionForIndex(position, index);
  }

  inline QRect rectForIndex(const QModelIndex & index) const {
    return QListView::rectForIndex(index);
  }

Q_SIGNALS:
  void activatedFiltered(const QModelIndex &index);

private Q_SLOTS:
  void activation(const QModelIndex &index);

private:
  bool activationAllowed_;
};

class FolderViewTreeView : public QTreeView {
  Q_OBJECT
public:
  friend class FolderView;
  FolderViewTreeView(QWidget* parent = 0);
  virtual ~FolderViewTreeView();
  virtual void setModel(QAbstractItemModel* model);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dragMoveEvent(QDragMoveEvent* e);
  virtual void dragLeaveEvent(QDragLeaveEvent* e);
  virtual void dropEvent(QDropEvent* e);
  
  virtual void rowsInserted(const QModelIndex& parent,int start, int end);
  virtual void rowsAboutToBeRemoved(const QModelIndex& parent,int start, int end);
  virtual void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

  virtual void resizeEvent(QResizeEvent* event);
  void queueLayoutColumns();

Q_SIGNALS:
  void activatedFiltered(const QModelIndex &index);

private Q_SLOTS:
  void layoutColumns();
  void activation(const QModelIndex &index);
  void onSortFilterChanged();

private:
  bool doingLayout_;
  QTimer* layoutTimer_;
  bool activationAllowed_;
};


} // namespace Fm

#endif // FM_FOLDERVIEW_P_H
