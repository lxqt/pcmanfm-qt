/*
 * Copyright 2014 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#include "dirtreemodel.h"
#include "dirtreemodelitem.h"
#include <QDebug>

namespace Fm {

DirTreeModel::DirTreeModel(QObject* parent) {
}

DirTreeModel::~DirTreeModel() {
}

// QAbstractItemModel implementation

QVariant DirTreeModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.column() > 1) {
    return QVariant();
  }
  DirTreeModelItem* item = itemFromIndex(index);
  if(item) {
    FmFileInfo* info = item->fileInfo_;
    switch(role) {
    case Qt::ToolTipRole:
      return QVariant(item->displayName_);
    case Qt::DisplayRole:
      return QVariant(item->displayName_);
    case Qt::DecorationRole:
      return QVariant(item->icon_);
    case FileInfoRole:
      return qVariantFromValue((void*)info);
    }
  }
  return QVariant();
}

int DirTreeModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

int DirTreeModel::rowCount(const QModelIndex& parent) const {
  if(!parent.isValid())
    return rootItems_.count();
  DirTreeModelItem* item = itemFromIndex(parent);
  if(item)
    return item->children_.count();
  return 0;
}

QModelIndex DirTreeModel::parent(const QModelIndex& child) const {
  DirTreeModelItem* item = itemFromIndex(child);
  if(item && item->parent_) {
    item = item->parent_; // go to parent item
    const QList<DirTreeModelItem*>& items = item->parent_ ? item->parent_->children_ : rootItems_;
    int row = items.indexOf(item); // this is Q(n) and may be slow :-(
    if(row >= 0)
      return createIndex(row, 1, (void*)item);
  }
  return QModelIndex();
}

QModelIndex DirTreeModel::index(int row, int column, const QModelIndex& parent) const {
  if(row >= 0 && column >= 0 && column == 0) {
    if(!parent.isValid()) { // root items
      if(row < rootItems_.count()) {
        const DirTreeModelItem* item = rootItems_.at(row);
        return createIndex(row, column, (void*)item);
      }
    }
    else { // child items
      DirTreeModelItem* parentItem = itemFromIndex(parent);
      if(row < parentItem->children_.count()) {
        const DirTreeModelItem* item = parentItem->children_.at(row);
        return createIndex(row, column, (void*)item);
      }
    }
  }
  return QModelIndex(); // invalid index
}

bool DirTreeModel::hasChildren(const QModelIndex& parent) const {
  // DirTreeModelItem* item = itemFromIndex(parent);
  // return item ? !item->children_.isEmpty() : NULL;
  return true;
}

QModelIndex DirTreeModel::indexFromItem(DirTreeModelItem* item) const {
  Q_ASSERT(item);
  const QList<DirTreeModelItem*>& items = item->parent_ ? item->parent_->children_ : rootItems_;
  int row = items.indexOf(item);
  if(row >= 0)
    return createIndex(row, 1, (void*)item);
  return QModelIndex();
}

// public APIs
QModelIndex DirTreeModel::addRoot(FmFileInfo* root) {
  DirTreeModelItem* item = new DirTreeModelItem(root, this);
  int row = rootItems_.count();
  beginInsertRows(QModelIndex(), row, row);
  item->fileInfo_ = fm_file_info_ref(root);
  rootItems_.append(item);
  // add_place_holder_child_item(model, item_l, NULL, FALSE);
  endInsertRows();
  return QModelIndex();
}

DirTreeModelItem* DirTreeModel::itemFromIndex(const QModelIndex& index) const {
  return reinterpret_cast<DirTreeModelItem*>(index.internalPointer());
}

/* Add file info to parent node to proper position.
 * GtkTreePath tp is the tree path of parent node. */
DirTreeModelItem* DirTreeModel::insertFileInfo(FmFileInfo* fi, Fm::DirTreeModelItem* parentItem) {
  qDebug() << "insertFileInfo: " << fm_file_info_get_disp_name(fi);
  DirTreeModelItem* item = new DirTreeModelItem(fi, this);
  insertItem(item, parentItem);
  return item;
}

// find a good position to insert the new item
int DirTreeModel::insertItem(DirTreeModelItem* newItem, DirTreeModelItem* parentItem) {
  if(!parentItem) {
    // FIXME: handle root items
    return -1;
  }
  const char* new_key = fm_file_info_get_collate_key(newItem->fileInfo_);
  int pos = 0;
  QList<DirTreeModelItem*>::iterator it;
  for(it = parentItem->children_.begin(); it != parentItem->children_.end(); ++it) {
    DirTreeModelItem* child = *it;
    if(G_UNLIKELY(!child->fileInfo_))
      continue;
    const char* key = fm_file_info_get_collate_key(child->fileInfo_);
    if(strcmp(new_key, key) <= 0)
      break;
    ++pos;
  }
  // inform the world that we're about to insert the item
  QModelIndex parentIndex = indexFromItem(parentItem);
  beginInsertRows(parentIndex, pos, pos);
  newItem->parent_ = parentItem;
  parentItem->children_.insert(it, newItem);
  endInsertRows();

  return pos;
}

void DirTreeModel::loadRow(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  Q_ASSERT(item);
  if(item) {
    if(!item->expanded_) {
      /* dynamically load content of the folder. */
      FmFolder* folder = fm_folder_from_path(fm_file_info_get_path(item->fileInfo_));
      item->folder_ = folder;
      /* g_debug("fm_dir_tree_model_load_row()"); */
      /* associate the data with loaded handler */
      g_signal_connect(folder, "finish-loading", G_CALLBACK(onFolderFinishLoading), item);
      g_signal_connect(folder, "files-added", G_CALLBACK(onFolderFilesAdded), item);
      g_signal_connect(folder, "files-removed", G_CALLBACK(onFolderFilesRemoved), item);
      g_signal_connect(folder, "files-changed", G_CALLBACK(onFolderFilesChanged), item);

      if(!item->children_.isEmpty()) {
        // add_place_holder_child_item(model, item_l, tp, TRUE);
      }
      /* set 'expanded' flag beforehand as callback may check it */
      item->expanded_ = true;
      /* if the folder is already loaded, call "loaded" handler ourselves */
      if(fm_folder_is_loaded(folder)) { // already loaded
        GList* file_l;
        FmFileInfoList* files = fm_folder_get_files(folder);
        for(file_l = fm_file_info_list_peek_head_link(files); file_l; file_l = file_l->next) {
          FmFileInfo* fi = FM_FILE_INFO(file_l->data);
          if(fm_file_info_is_dir(fi)) {
            insertFileInfo(fi, item);
          }
        }
        onFolderFinishLoading(folder, item);
      }
    }
  }
}

void DirTreeModel::unloadRow(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  if(item) {
#if 0
    GList* item_l = (GList*)it->user_data;
    DirTreeModelItem* item = (DirTreeModelItem*)item_l->data;
    g_return_if_fail(item != NULL);
    if(item->expanded) { /* do some cleanup */
      GList* had_children = item->children;
      /* remove all children, and replace them with a dummy child
       * item to keep expander in the tree view around. */
      remove_all_children(model, item_l, tp);

      /* now, GtkTreeView think that we have no child since all
       * child items are removed. So we add a place holder child
       * item to keep the expander around. */
      /* don't leave expanders if not stated in config */
      if(had_children)
        add_place_holder_child_item(model, item_l, tp, TRUE);
      /* deactivate folder since it will be reactivated on expand */
      item_free_folder(item->folder, item_l);
      item->folder = NULL;
      item->expanded = FALSE;
      item->loaded = FALSE;
      /* g_debug("fm_dir_tree_model_unload_row()"); */
    }
#endif
  }
}

bool DirTreeModel::isLoaded(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? item->loaded_ : false;
}

QIcon DirTreeModel::icon(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? item->icon_ : QIcon();
}

FmFileInfo* DirTreeModel::fileInfo(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? item->fileInfo_ : NULL;
}

FmPath* DirTreeModel::filePath(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? fm_file_info_get_path(item->fileInfo_) : NULL;
}

const char* DirTreeModel::dispName(const QModelIndex& index) {
  DirTreeModelItem* item = itemFromIndex(index);
  return item ? fm_file_info_get_disp_name(item->fileInfo_) : NULL;
}

void DirTreeModel::setShowHidden(bool show_hidden) {
  showHidden_ = show_hidden;
}

// FmFolder signal handlers

// static
void DirTreeModel::onFolderFinishLoading(FmFolder* folder, gpointer user_data) {
  DirTreeModelItem* item = (DirTreeModelItem*)user_data;
  DirTreeModel* _this = item->model_;
  QModelIndex index = _this->indexFromItem(item);
  /* set 'loaded' flag beforehand as callback may check it */
  item->loaded_ = true;
#if 0
  GList* place_holder_l;
  place_holder_l = item->children;
  /* don't leave expanders if not stated in config */
  /* if we have loaded sub dirs, remove the place holder */
  if(fm_config->no_child_non_expandable || !place_holder_l || place_holder_l->next) {
    /* remove the fake placeholder item showing "Loading..." */
    /* #3614965: crash after removing only child from existing directory:
       after reload first item may be absent or may be not a placeholder,
       if no_child_non_expandable is unset, place_holder_l cannot be NULL */
    if(place_holder_l && ((DirTreeModelItem*)place_holder_l->data)->fi == NULL)
      remove_item(model, place_holder_l);
    /* in case if no_child_non_expandable was unset while reloading, it may
       be still place_holder_l is NULL but let leave empty folder still */
  }
  else { /* if we have no sub dirs, leave the place holder and let it show "Empty" */
  }
#endif
  Q_EMIT _this->rowLoaded(index);
}

// static
void DirTreeModel::onFolderFilesAdded(FmFolder* folder, GSList* files, gpointer user_data) {
  GSList* l;
  DirTreeModelItem* item = (DirTreeModelItem*)user_data;
  DirTreeModel* _this = item->model_;
  for(l = files; l; l = l->next) {
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    if(fm_file_info_is_dir(fi)) { /* FIXME: maybe adding files can be allowed later */
      /* Ideally FmFolder should not emit files-added signals for files that
       * already exists. So there is no need to check for duplication here. */
      _this->insertFileInfo(fi, item);
    }
  }
}

// static
void DirTreeModel::onFolderFilesRemoved(FmFolder* folder, GSList* files, gpointer user_data) {
  GSList* l;
  DirTreeModelItem* item = (DirTreeModelItem*)user_data;
  DirTreeModel* _this = item->model_;
#if 0
  for(l = files; l; l = l->next) {
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    FmPath* path = fm_file_info_get_path(fi);
    GList* rm_item_l = children_by_name(model, item->children,
                                        fm_path_get_basename(path), NULL);
    if(rm_item_l)
      remove_item(model, rm_item_l);
  }
#endif
}

// static
void DirTreeModel::onFolderFilesChanged(FmFolder* folder, GSList* files, gpointer user_data) {
  GSList* l;
  DirTreeModelItem* item = (DirTreeModelItem*)user_data;
  DirTreeModel* _this = item->model_;
#if 0
  GtkTreePath* tp = item_to_tree_path(model, item_l);

  /* g_debug("files changed!!"); */

  for(l = files; l; l = l->next) {
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    int idx;
    FmPath* path = fm_file_info_get_path(fi);
    GList* changed_item_l = children_by_name(model, item->children,
                            fm_path_get_basename(path), &idx);
    /* g_debug("changed file: %s", fi->path->name); */
    if(changed_item_l) {
      DirTreeModelItem* changed_item = (DirTreeModelItem*)changed_item_l->data;
      GtkTreeIter it;
      if(changed_item->fi)
        fm_file_info_unref(changed_item->fi);
      changed_item->fi = fm_file_info_ref(fi);
      /* inform gtk tree view about the change */
      item_to_tree_iter(model, changed_item_l, &it);
      gtk_tree_path_append_index(tp, idx);
      gtk_tree_model_row_changed(GTK_TREE_MODEL(model), tp, &it);
      gtk_tree_path_up(tp);

      /* FIXME and TODO: check if we have sub folder */
      /* item_queue_subdir_check(model, changed_item_l); */
    }
  }
  gtk_tree_path_free(tp);
#endif
}


} // namespace Fm
