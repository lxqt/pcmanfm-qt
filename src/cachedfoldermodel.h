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


#ifndef FM_CACHEDFOLDERMODEL_H
#define FM_CACHEDFOLDERMODEL_H

#include "libfmqtglobals.h"
#include "foldermodel.h"

namespace Fm {

class LIBFM_QT_API CachedFolderModel : public FolderModel {
  Q_OBJECT
public:
  CachedFolderModel(FmFolder* folder);
  void ref() {
    ++refCount;
  }
  void unref();

  static CachedFolderModel* modelFromFolder(FmFolder* folder);
  static CachedFolderModel* modelFromPath(FmPath* path);

private:
  virtual ~CachedFolderModel();
  void setFolder(FmFolder* folder);
private:
  int refCount;
};


}

#endif // FM_CACHEDFOLDERMODEL_H
