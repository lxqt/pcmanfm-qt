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
#include <QDebug>
#include <QItemSelection>
#include <QGuiApplication>
#include <QMouseEvent>
#include "dirtreemodel.h"
#include "dirtreemodelitem.h"
#include "filemenu.h"

namespace Fm {

DirTreeView::DirTreeView(QWidget* parent):
  currentExpandingItem_(NULL),
  currentPath_(NULL) {

  setSelectionMode(QAbstractItemView::SingleSelection);
  setHeaderHidden(true);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  header()->setStretchLastSection(false);

  connect(this, &DirTreeView::collapsed, this, &DirTreeView::onCollapsed);
  connect(this, &DirTreeView::expanded, this, &DirTreeView::onExpanded);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &DirTreeView::customContextMenuRequested,
          this, &DirTreeView::onCustomContextMenuRequested);
}

DirTreeView::~DirTreeView() {
  if(currentPath_)
    fm_path_unref(currentPath_);
}

void DirTreeView::cancelPendingChdir() {
  if(!pathsToExpand_.isEmpty()) {
    pathsToExpand_.clear();
    if(!currentExpandingItem_)
      return;
    DirTreeModel* _model = static_cast<DirTreeModel*>(model());
    disconnect(_model, &DirTreeModel::rowLoaded, this, &DirTreeView::onRowLoaded);
    currentExpandingItem_ = NULL;
  }
}

void DirTreeView::expandPendingPath() {
  if(pathsToExpand_.isEmpty())
    return;

  FmPath* path = pathsToExpand_.first().data();
  // qDebug() << "expanding: " << Path(path).displayBasename();
  DirTreeModel* _model = static_cast<DirTreeModel*>(model());
  DirTreeModelItem* item = _model->itemFromPath(path);
  // qDebug() << "findItem: " << item;
  if(item) {
    currentExpandingItem_ = item;
    connect(_model, &DirTreeModel::rowLoaded, this, &DirTreeView::onRowLoaded);
    if(item->loaded_) { // the node is already loaded
      onRowLoaded(item->index());
    }
    else {
      // _model->loadRow(item->index());
      item->loadFolder();
    }
  }
  else {
    selectionModel()->clear();
    /* since we never get it loaded we need to update cwd here */
    if(currentPath_)
      fm_path_unref(currentPath_);
    currentPath_ = fm_path_ref(path);

    cancelPendingChdir(); // FIXME: is this correct? this is not done in the gtk+ version of libfm.
  }
}

void DirTreeView::onRowLoaded(const QModelIndex& index) {
  DirTreeModel* _model = static_cast<DirTreeModel*>(model());
  if(!currentExpandingItem_)
    return;
  if(currentExpandingItem_ != _model->itemFromIndex(index)) {
    return;
  }
  /* disconnect the handler since we only need it once */
  disconnect(_model, &DirTreeModel::rowLoaded, this, &DirTreeView::onRowLoaded);

  // DirTreeModelItem* item = _model->itemFromIndex(index);
  // qDebug() << "row loaded: " << item->displayName_;
  /* after the folder is loaded, the files should have been added to
    * the tree model */
  expand(index);

  /* remove the expanded path from pending list */
  pathsToExpand_.removeFirst();
  if(pathsToExpand_.isEmpty()) {  /* this is the last one and we're done, select the item */
    // qDebug() << "Done!";
    selectionModel()->select(index, QItemSelectionModel::SelectCurrent|QItemSelectionModel::Clear);
    scrollTo(index, QAbstractItemView::EnsureVisible);
  }
  else { /* continue expanding next pending path */
    expandPendingPath();
  }
}


void DirTreeView::setCurrentPath(FmPath* path) {
  DirTreeModel* _model = static_cast<DirTreeModel*>(model());
  if(!_model)
    return;
  int rowCount = _model->rowCount(QModelIndex());
  if(rowCount <= 0 || fm_path_equal(currentPath_, path))
    return;

  if(currentPath_)
    fm_path_unref(currentPath_);
  currentPath_ = fm_path_ref(path);

  // NOTE: The content of each node is loaded on demand dynamically.
  // So, when we ask for a chdir operation, some nodes do not exists yet.
  // We have to wait for the loading of child nodes and continue the
  // pending chdir operation after the child nodes become available.

  // cancel previous pending tree expansion
  cancelPendingChdir();

  /* find a root item containing this path */
  FmPath* root;
  for(int row = 0; row < rowCount; ++row) {
    QModelIndex index = _model->index(row, 0, QModelIndex());
    root = _model->filePath(index);
    if(fm_path_has_prefix(path, root))
      break;
    root = NULL;
  }

  if(root) { /* root item is found */
    do { /* add path elements one by one to a list */
      pathsToExpand_.prepend(path);
      // qDebug() << "prepend path: " << Path(path).displayBasename();
      if(fm_path_equal(path, root))
        break;
      path = fm_path_get_parent(path);
    }
    while(path);

    expandPendingPath();
  }
}

void DirTreeView::setModel(QAbstractItemModel* model) {
  Q_ASSERT(model->inherits("Fm::DirTreeModel"));

  if(!pathsToExpand_.isEmpty()) // if a chdir request is in progress, cancel it
    cancelPendingChdir();

  QTreeView::setModel(model);
  header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &DirTreeView::onSelectionChanged);
}

void DirTreeView::mousePressEvent(QMouseEvent* event) {
  if(event && event->button() == Qt::RightButton &&
     event->type() == QEvent::MouseButtonPress) {
    // Do not change the selection when the context menu is activated.
    return;
  }
  QTreeView::mousePressEvent(event);
}

void DirTreeView::onCustomContextMenuRequested(const QPoint& pos) {
  QModelIndex index = indexAt(pos);
  if(index.isValid()) {
    QVariant data = index.data(DirTreeModel::FileInfoRole);
    FmFileInfo* fileInfo = reinterpret_cast<FmFileInfo*>(data.value<void*>());
    if(fileInfo) {
      FmPath* path = fm_file_info_get_path(fileInfo);
      FmFileInfoList* files = fm_file_info_list_new();
      fm_file_info_list_push_tail(files, fileInfo);
      Fm::FileMenu* menu = new Fm::FileMenu(files, fileInfo, path);
      // FIXME: apply some settings to the menu and set a proper file launcher to it
      Q_EMIT prepareFileMenu(menu);
      fm_file_info_list_unref(files);
      QVariant pathData = qVariantFromValue(reinterpret_cast<void*>(path));
      QAction* action = menu->openAction();
      action->disconnect();
      action->setData(index);
      connect(action, &QAction::triggered, this, &DirTreeView::onOpen);
      action = new QAction(QIcon::fromTheme("window-new"), tr("Open in New T&ab"), menu);
      action->setData(pathData);
      connect(action, &QAction::triggered, this, &DirTreeView::onNewTab);
      menu->insertAction(menu->separator1(), action);
      action = new QAction(QIcon::fromTheme("window-new"), tr("Open in New Win&dow"), menu);
      action->setData(pathData);
      connect(action, &QAction::triggered, this, &DirTreeView::onNewWindow);
      menu->insertAction(menu->separator1(), action);
      if(fm_file_info_is_native(fileInfo)) {
        action = new QAction(QIcon::fromTheme("utilities-terminal"), tr("Open in Termina&l"), menu);
        action->setData(pathData);
        connect(action, &QAction::triggered, this, &DirTreeView::onOpenInTerminal);
        menu->insertAction(menu->separator1(), action);
      }
      menu->exec(mapToGlobal(pos));
      delete menu;
    }
  }
}

void DirTreeView::onOpen() {
  if(QAction* action = qobject_cast<QAction*>(sender())) {
    setCurrentIndex(action->data().toModelIndex());
  }
}

void DirTreeView::onNewWindow() {
  if(QAction* action = qobject_cast<QAction*>(sender())) {
    FmPath* path = reinterpret_cast<FmPath*>(action->data().value<void*>());
    Q_EMIT openFolderInNewWindowRequested(path);
  }
}

void DirTreeView::onNewTab() {
  if(QAction* action = qobject_cast<QAction*>(sender())) {
    FmPath* path = reinterpret_cast<FmPath*>(action->data().value<void*>());
    Q_EMIT openFolderInNewTabRequested(path);
  }
}

void DirTreeView::onOpenInTerminal() {
  if(QAction* action = qobject_cast<QAction*>(sender())) {
    FmPath* path = reinterpret_cast<FmPath*>(action->data().value<void*>());
    Q_EMIT openFolderInTerminalRequested(path);
  }
}

void DirTreeView::onNewFolder() {
  if(QAction* action = qobject_cast<QAction*>(sender())) {
    FmPath* path = reinterpret_cast<FmPath*>(action->data().value<void*>());
    Q_EMIT createNewFolderRequested(path);
  }
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

void DirTreeView::onSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected) {
  if(!selected.isEmpty()) {
    QModelIndex index = selected.first().topLeft();
    DirTreeModel* _model = static_cast<DirTreeModel*>(model());
    FmPath* path = _model->filePath(index);
    if(path && currentPath_ && fm_path_equal(path, currentPath_))
      return;
    cancelPendingChdir();
    if(!path)
      return;
    if(currentPath_)
      fm_path_unref(currentPath_);
    currentPath_ = fm_path_ref(path);

    // FIXME: use enums for type rather than hard-coded values 0 or 1
    int type = 0;
    if(QGuiApplication::mouseButtons() & Qt::MiddleButton)
      type = 1;
    Q_EMIT chdirRequested(type, path);
  }
}


} // namespace Fm
