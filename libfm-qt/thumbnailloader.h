/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  PCMan <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef FM_THUMBNAILLOADER_H
#define FM_THUMBNAILLOADER_H

#include "libfmqtglobals.h"
#include <QImage>
#include <libfm/fm.h>
#include <gio/gio.h>

namespace Fm {

class LIBFM_QT_API ThumbnailLoader {

public:
  ThumbnailLoader();
  virtual ~ThumbnailLoader();
  
  static ThumbnailLoader* instance() {
    return theThumbnailLoader;
  }

  static FmThumbnailLoader* load(FmFileInfo* fileInfo, int size, FmThumbnailLoaderCallback callback, gpointer user_data) {
    // qDebug("load thumbnail: %s", fm_file_info_get_disp_name(fileInfo));
    return fm_thumbnail_loader_load(fileInfo, size, callback, user_data);
  }

  static FmFileInfo* fileInfo(FmThumbnailLoader* result) {
    return fm_thumbnail_loader_get_file_info(result);
  }

  static void cancel(FmThumbnailLoader* result) {
    fm_thumbnail_loader_cancel(result);
  }

  static QImage image(FmThumbnailLoader* result);

  static int size(FmThumbnailLoader* result) {
    return fm_thumbnail_loader_get_size(result);
  }

  static void setLocalFilesOnly(bool value) {
    localFilesOnly_ = value;
    if(fm_config)
      fm_config->thumbnail_local = localFilesOnly_;
  }

  static bool localFilesOnly() {
    return localFilesOnly_;
  }

  static int maxThumbnailFileSize() {
    return maxThumbnailFileSize_;
  }
  
  static void setMaxThumbnailFileSize(int size) {
    maxThumbnailFileSize_ = size;
    if(fm_config)
      fm_config->thumbnail_max = maxThumbnailFileSize_;
  }

private:
  static GObject* readImageFromFile(const char* filename);
  static GObject* readImageFromStream(GInputStream* stream, guint64 len, GCancellable* cancellable);
  static gboolean writeImage(GObject* image, const char* filename);
  static GObject* scaleImage(GObject* ori_pix, int new_width, int new_height);
  static int getImageWidth(GObject* image);
  static int getImageHeight(GObject* image);
  static char* getImageText(GObject* image, const char* key);
  static gboolean setImageText(GObject* image, const char* key, const char* val);
  static GObject* rotateImage(GObject* image, int degree);

private:
  static ThumbnailLoader* theThumbnailLoader;
  static bool localFilesOnly_;
  static int maxThumbnailFileSize_;
};

}

#endif // FM_THUMBNAILLOADER_H
