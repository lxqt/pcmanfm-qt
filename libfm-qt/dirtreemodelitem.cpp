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
  folder_(NULL),
  expanded_(false),
  loaded_(false),
  fileInfo_(NULL),
  placeHolderChild_(NULL),
  parent_(NULL) {
}

DirTreeModelItem::DirTreeModelItem(FmFileInfo* info, DirTreeModel* model, DirTreeModelItem* parent):
  model_(model),
  folder_(NULL),
  expanded_(false),
  loaded_(false),
  fileInfo_(fm_file_info_ref(info)),
  displayName_(QString::fromUtf8(fm_file_info_get_disp_name(info))),
  icon_(IconTheme::icon(fm_file_info_get_icon(info))),
  placeHolderChild_(NULL),
  parent_(parent) {

  if(info)
    addPlaceHolderChild();
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

void DirTreeModelItem::addPlaceHolderChild() {
  placeHolderChild_ = new DirTreeModelItem();
  placeHolderChild_->parent_ = this;
  placeHolderChild_->model_ = model_;
  children_.append(placeHolderChild_);
}

void DirTreeModelItem::freeFolder() {
  if(folder_) {
    g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderFinishLoading), this);
    g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderFilesAdded), this);
    g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderFilesRemoved), this);
    g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderFilesChanged), this);
    g_object_unref(folder_);
    folder_ = NULL;
  }
}

void DirTreeModelItem::loadFolder() {
  if(!expanded_) {
    // the place holder child item
    placeHolderChild_->displayName_ = DirTreeModel::tr("Loading...");

    /* dynamically load content of the folder. */
    folder_ = fm_folder_from_path(fm_file_info_get_path(fileInfo_));
    /* g_debug("fm_dir_tree_model_load_row()"); */
    /* associate the data with loaded handler */
    g_signal_connect(folder_, "finish-loading", G_CALLBACK(onFolderFinishLoading), this);
    g_signal_connect(folder_, "files-added", G_CALLBACK(onFolderFilesAdded), this);
    g_signal_connect(folder_, "files-removed", G_CALLBACK(onFolderFilesRemoved), this);
    g_signal_connect(folder_, "files-changed", G_CALLBACK(onFolderFilesChanged), this);

    /* set 'expanded' flag beforehand as callback may check it */
    expanded_ = true;
    /* if the folder is already loaded, call "loaded" handler ourselves */
    if(fm_folder_is_loaded(folder_)) { // already loaded
      GList* file_l;
      FmFileInfoList* files = fm_folder_get_files(folder_);
      for(file_l = fm_file_info_list_peek_head_link(files); file_l; file_l = file_l->next) {
	FmFileInfo* fi = FM_FILE_INFO(file_l->data);
	if(fm_file_info_is_dir(fi)) {
	  model_->insertFileInfo(fi, this);
	}
      }
      onFolderFinishLoading(folder_, this);
    }
  }
}

void DirTreeModelItem::unloadFolder() {
  if(expanded_) { /* do some cleanup */
    /* remove all children, and replace them with a dummy child
      * item to keep expander in the tree view around. */

    // delete all child items
    QModelIndex index = model_->indexFromItem(this);
    model_->beginRemoveRows(index, 0, children_.count() - 1);
    if(!children_.isEmpty()) {
      Q_FOREACH(DirTreeModelItem* item, children_) {
	delete item;
      }
      children_.clear();
    }
    model_->endRemoveRows();
    /* now, we have no child since all child items are removed.
     * So we add a place holder child item to keep the expander around. */
    addPlaceHolderChild();
    /* deactivate folder since it will be reactivated on expand */
    freeFolder();
    expanded_ = false;
    loaded_ = false;
  }
}

// FmFolder signal handlers

// static
void DirTreeModelItem::onFolderFinishLoading(FmFolder* folder, gpointer user_data) {
  DirTreeModelItem* _this = (DirTreeModelItem*)user_data;
  DirTreeModel* model = _this->model_;
  QModelIndex index = model->indexFromItem(_this);
  /* set 'loaded' flag beforehand as callback may check it */
  _this->loaded_ = true;

  // remove the placeholder child if needed
  if(_this->children_.count() == 1) { // we have no other child other than the place holder item, leave it
    _this->placeHolderChild_->displayName_ = DirTreeModel::tr("<No sub folders>");
    QModelIndex index = model->indexFromItem(_this->placeHolderChild_);
    Q_EMIT model->dataChanged(index, index);
  }
  else {
    QModelIndex index = model->indexFromItem(_this);
    int pos = _this->children_.indexOf(_this->placeHolderChild_);
    model->beginRemoveRows(index,pos, pos); 
    _this->children_.removeAt(pos);
    delete _this->placeHolderChild_;
    model->endRemoveRows(); 
    _this->placeHolderChild_ = NULL;
  }

  Q_EMIT model->rowLoaded(index);
}

// static
void DirTreeModelItem::onFolderFilesAdded(FmFolder* folder, GSList* files, gpointer user_data) {
  GSList* l;
  DirTreeModelItem* _this = (DirTreeModelItem*)user_data;
  DirTreeModel* model = _this->model_;

  // FIXME: only emit rowInserted signals does not seem to work in Qt after
  // we insert new items in an expanded row. Ask for a re-layout here.
  Q_EMIT model->layoutAboutToBeChanged();
  for(l = files; l; l = l->next) {
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    if(fm_file_info_is_dir(fi)) { /* FIXME: maybe adding files can be allowed later */
      /* Ideally FmFolder should not emit files-added signals for files that
       * already exists. So there is no need to check for duplication here. */
      model->insertFileInfo(fi, _this);
    }
  }
  Q_EMIT model->layoutChanged();
}

// static
void DirTreeModelItem::onFolderFilesRemoved(FmFolder* folder, GSList* files, gpointer user_data) {
  DirTreeModelItem* _this = (DirTreeModelItem*)user_data;
  DirTreeModel* model = _this->model_;

  for(GSList* l = files; l; l = l->next) {
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    int pos;
    DirTreeModelItem* child  = _this->childFromName(fm_file_info_get_name(fi), &pos);
    if(child) {
      QModelIndex index = model->indexFromItem(_this);
      model->beginRemoveRows(index, pos, pos);
      _this->children_.removeAt(pos);
      delete child;
      model->endRemoveRows();
    }
  }
}

// static
void DirTreeModelItem::onFolderFilesChanged(FmFolder* folder, GSList* files, gpointer user_data) {
  DirTreeModelItem* _this = (DirTreeModelItem*)user_data;
  DirTreeModel* model = _this->model_;

  for(GSList* l = files; l; l = l->next) {
    FmFileInfo* changedFile = FM_FILE_INFO(l->data);
    int pos;
    DirTreeModelItem* child = _this->childFromName(fm_file_info_get_name(changedFile), &pos);
    if(child) {
      QModelIndex childIndex = model->indexFromItem(child);
      Q_EMIT model->dataChanged(childIndex, childIndex);
    }
  }
}

DirTreeModelItem* DirTreeModelItem::childFromName(const char* utf8_name, int* pos) {
  int i = 0;
  Q_FOREACH(DirTreeModelItem* item, children_) {
    if(item->fileInfo_ && strcmp(fm_file_info_get_name(item->fileInfo_), utf8_name) == 0) {
      if(pos)
	*pos = i;
      return item;
    }
    ++i;
  }
  return NULL;
}


} // namespace Fm

