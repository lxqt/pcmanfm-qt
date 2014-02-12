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

#include "dirtreemodelitem.h"
#include "dirtreemodel.h"
#include "icontheme.h"

namespace Fm {

DirTreeModelItem::DirTreeModelItem():
  model_(NULL),
  expanded_(false),
  loaded_(false),
  fileInfo_(NULL),
  parent_(NULL) {
}

DirTreeModelItem::DirTreeModelItem(FmFileInfo* info, DirTreeModel* model, DirTreeModelItem* parent):
  model_(model),
  expanded_(false),
  loaded_(false),
  fileInfo_(fm_file_info_ref(info)),
  displayName_(QString::fromUtf8(fm_file_info_get_disp_name(info))),
  icon_(IconTheme::icon(fm_file_info_get_icon(info))),
  parent_(parent) {
}

DirTreeModelItem::~DirTreeModelItem() {
  if(fileInfo_)
    fm_file_info_unref(fileInfo_);

  if(folder_)
    freeFolder();

  // delete child items if needed
  if(!children_.isEmpty()) {
    Q_FOREACH(DirTreeModelItem* item, children_) {
      delete item;
    }
  }
}

void DirTreeModelItem::freeFolder() {
  g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderFinishLoading), this);
  g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderFilesAdded), this);
  g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderFilesRemoved), this);
  g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderFilesChanged), this);
  g_object_unref(folder_);
}

void DirTreeModelItem::loadFolder() {
  if(!expanded_) {
    /* dynamically load content of the folder. */
    FmFolder* folder = fm_folder_from_path(fm_file_info_get_path(fileInfo_));
    folder_ = folder;
    /* g_debug("fm_dir_tree_model_load_row()"); */
    /* associate the data with loaded handler */
    g_signal_connect(folder, "finish-loading", G_CALLBACK(onFolderFinishLoading), this);
    g_signal_connect(folder, "files-added", G_CALLBACK(onFolderFilesAdded), this);
    g_signal_connect(folder, "files-removed", G_CALLBACK(onFolderFilesRemoved), this);
    g_signal_connect(folder, "files-changed", G_CALLBACK(onFolderFilesChanged), this);

    if(!children_.isEmpty()) {
      // add_place_holder_child_item(model, item_l, tp, TRUE);
    }
    /* set 'expanded' flag beforehand as callback may check it */
    expanded_ = true;
    /* if the folder is already loaded, call "loaded" handler ourselves */
    if(fm_folder_is_loaded(folder)) { // already loaded
      GList* file_l;
      FmFileInfoList* files = fm_folder_get_files(folder);
      for(file_l = fm_file_info_list_peek_head_link(files); file_l; file_l = file_l->next) {
	FmFileInfo* fi = FM_FILE_INFO(file_l->data);
	if(fm_file_info_is_dir(fi)) {
	  model_->insertFileInfo(fi, this);
	}
      }
      onFolderFinishLoading(folder, this);
    }
  }
}

void DirTreeModelItem::unloadFolder() {
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

// FmFolder signal handlers

// static
void DirTreeModelItem::onFolderFinishLoading(FmFolder* folder, gpointer user_data) {
  DirTreeModelItem* _this = (DirTreeModelItem*)user_data;
  DirTreeModel* model = _this->model_;
  QModelIndex index = model->indexFromItem(_this);
  /* set 'loaded' flag beforehand as callback may check it */
  _this->loaded_ = true;
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
  Q_EMIT model->rowLoaded(index);
}

// static
void DirTreeModelItem::onFolderFilesAdded(FmFolder* folder, GSList* files, gpointer user_data) {
  GSList* l;
  DirTreeModelItem* _this = (DirTreeModelItem*)user_data;
  DirTreeModel* model = _this->model_;

  for(l = files; l; l = l->next) {
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    if(fm_file_info_is_dir(fi)) { /* FIXME: maybe adding files can be allowed later */
      /* Ideally FmFolder should not emit files-added signals for files that
       * already exists. So there is no need to check for duplication here. */
      model->insertFileInfo(fi, _this);
    }
  }
}

// static
void DirTreeModelItem::onFolderFilesRemoved(FmFolder* folder, GSList* files, gpointer user_data) {
  GSList* l;
  DirTreeModelItem* _this = (DirTreeModelItem*)user_data;
  DirTreeModel* model = _this->model_;

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
void DirTreeModelItem::onFolderFilesChanged(FmFolder* folder, GSList* files, gpointer user_data) {
  GSList* l;
  DirTreeModelItem* _this = (DirTreeModelItem*)user_data;
  DirTreeModel* model = _this->model_;

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

