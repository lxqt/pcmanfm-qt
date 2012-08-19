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


#include "folderview.h"
#include "foldermodel.h"
#include <QHeaderView>
#include <iostream>

using namespace Fm;

FolderView::FolderView(QWidget* parent): QTreeView(parent) { // QListView(parent) {
  setItemsExpandable(false);
  setRootIsDecorated(false);
  header()->setResizeMode(0, QHeaderView::ResizeToContents); // QHeaderView::Stretch);

  connect(this, SIGNAL(activated(QModelIndex)), SLOT(onItemActivated(QModelIndex)));
}

FolderView::~FolderView() {
}

void FolderView::onItemActivated(QModelIndex index) {
  QVariant data = index.model()->data(index, FolderModel::FileInfoRole);
  FmFileInfo* info = (FmFileInfo*)data.value<void*>();
  if(info) {
    Q_EMIT clicked(ACTIVATED, info);
  }
}

#include "folderview.moc"
