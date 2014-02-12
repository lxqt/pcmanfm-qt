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

#include "dirtreeview.h"
#include <QHeaderView>

using namespace Fm;

DirTreeView::DirTreeView(QWidget* parent):
  currentPath_(NULL) {

  setSelectionMode(QAbstractItemView::SingleSelection);
  setHeaderHidden(true);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  connect(this, SIGNAL(activated(QModelIndex)), SLOT(onActivated(QModelIndex)));
  connect(this, SIGNAL(clicked(QModelIndex)), SLOT(onClicked(QModelIndex)));
  connect(this, SIGNAL(collapsed(QModelIndex)), SLOT(onCollapsed(QModelIndex)));
  connect(this, SIGNAL(expanded(QModelIndex)), SLOT(onExpanded(QModelIndex)));
}

DirTreeView::~DirTreeView() {
  if(currentPath_)
    fm_path_unref(currentPath_);
}

void DirTreeView::setCurrentPath(FmPath* path) {
  if(currentPath_)
    fm_path_unref(currentPath_);
  currentPath_ = fm_path_ref(path);
}

void DirTreeView::setModel(QAbstractItemModel* model) {
  Q_ASSERT(model->inherits("Fm::DirTreeModel"));
  QTreeView::setModel(model);
  header()->setResizeMode(0, QHeaderView::ResizeToContents);
  header()->setStretchLastSection(true);
  connect(selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), SLOT(onCurrentRowChanged(QModelIndex,QModelIndex)));
}


void DirTreeView::contextMenuEvent(QContextMenuEvent* event) {
  QAbstractScrollArea::contextMenuEvent(event);
}

void DirTreeView::onActivated(const QModelIndex& index) {

}

void DirTreeView::onClicked(const QModelIndex& index) {

}

void DirTreeView::onCollapsed(const QModelIndex& index) {
  DirTreeModel* treeModel = static_cast<DirTreeModel*>(model());
  if(treeModel) {
    treeModel->unloadRow(index);
  }
}

void DirTreeView::onExpanded(const QModelIndex& index) {
  DirTreeModel* treeModel = static_cast<DirTreeModel*>(model());
  if(treeModel) {
    treeModel->loadRow(index);
  }
}

void DirTreeView::onCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous) {

}
