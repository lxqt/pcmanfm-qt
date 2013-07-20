/*
 * 
 *  Copyright (C) 2012  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef FM_FOLDERMODELITEM_H
#define FM_FOLDERMODELITEM_H

#include "libfmqtglobals.h"
#include <libfm/fm.h>
#include <QImage>
#include <QString>
#include <QIcon>
#include <QVector>

namespace Fm {

class LIBFM_QT_API FolderModelItem {
public:

  enum ThumbnailStatus {
    ThumbnailNotChecked,
    ThumbnailLoading,
    ThumbnailLoaded,
    ThumbnailFailed
  };

  struct Thumbnail {
    int size;
    ThumbnailStatus status;
    QImage image;
  };

public:
  FolderModelItem(FmFileInfo* _info);
  FolderModelItem(const FolderModelItem& other);
  virtual ~FolderModelItem();

  Thumbnail* findThumbnail(int size);
  // void setThumbnail(int size, QImage image);
  void removeThumbnail(int size);

  QString displayName;
  QIcon icon;
  FmFileInfo* info;
  QVector<Thumbnail> thumbnails;
};

}

#endif // FM_FOLDERMODELITEM_H
