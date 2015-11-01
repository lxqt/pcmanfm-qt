/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

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


#include "cachedfoldermodel.h"

using namespace Fm;

static GQuark data_id = 0;


CachedFolderModel::CachedFolderModel(FmFolder* folder):
  FolderModel(),
  refCount(1) {

  FolderModel::setFolder(folder);
}

CachedFolderModel::~CachedFolderModel() {
}

CachedFolderModel* CachedFolderModel::modelFromFolder(FmFolder* folder) {
  CachedFolderModel* model = NULL;
  if(!data_id)
    data_id = g_quark_from_static_string("CachedFolderModel");
  gpointer qdata = g_object_get_qdata(G_OBJECT(folder), data_id);
  model = reinterpret_cast<CachedFolderModel*>(qdata);
  if(model) {
    // qDebug("cache found!!");
    model->ref();
  }
  else {
    model = new CachedFolderModel(folder);
    g_object_set_qdata(G_OBJECT(folder), data_id, model);
  }
  return model;
}

CachedFolderModel* CachedFolderModel::modelFromPath(FmPath* path) {
  FmFolder* folder = fm_folder_from_path(path);
  if(folder) {
    CachedFolderModel* model = modelFromFolder(folder);
    g_object_unref(folder);
    return model;
  }
  return NULL;
}

void CachedFolderModel::unref() {
  // qDebug("unref cache");
  --refCount;
  if(refCount <= 0) {
    g_object_set_qdata(G_OBJECT(folder()), data_id, NULL);
    deleteLater();
  }
}

