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
#include "icontheme.h"

using namespace Fm;

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
  
  // delete child items if needed
  if(!children_.isEmpty()) {
    Q_FOREACH(DirTreeModelItem* item, children_) {
      delete item;
    }
  }
}


