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


#include "thumbnailloader.h"
#include <new>
#include <QByteArray>

using namespace Fm;

// FmQImageWrapper is a GObject used to wrap QImage objects and use in glib-based libfm
#define FM_TYPE_QIMAGE_WRAPPER              (fm_qimage_wrapper_get_type())
#define FM_QIMAGE_WRAPPER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj),\
FM_TYPE_QIMAGE_WRAPPER, FmQImageWrapper))
#define FM_QIMAGE_WRAPPER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),\
FM_TYPE_QIMAGE_WRAPPER, FmQImageWrapperClass))
#define FM_IS_QIMAGE_WRAPPER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
FM_TYPE_QIMAGE_WRAPPER))
#define FM_IS_QIMAGE_WRAPPER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass),\
FM_TYPE_QIMAGE_WRAPPER))
#define FM_QIMAGE_WRAPPER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),\
FM_TYPE_QIMAGE_WRAPPER, FmQImageWrapperClass))

typedef struct _FmQImageWrapper         FmQImageWrapper;
typedef struct _FmQImageWrapperClass        FmQImageWrapperClass;

struct _FmQImageWrapper {
  GObject parent;
  QImage image;
};

struct _FmQImageWrapperClass {
  GObjectClass parent_class;
};

GType       fm_qimage_wrapper_get_type(void);
GObject*    fm_qimage_wrapper_new(void);
static void fm_qimage_wrapper_finalize(GObject *self);

G_DEFINE_TYPE(FmQImageWrapper, fm_qimage_wrapper, G_TYPE_OBJECT)

static void fm_qimage_wrapper_class_init(FmQImageWrapperClass *klass) {
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = fm_qimage_wrapper_finalize;
}

static void fm_qimage_wrapper_init(FmQImageWrapper *self) {
  // placement new for QImage
  new(&self->image) QImage();
}

static void fm_qimage_wrapper_finalize(GObject *self) {
  FmQImageWrapper *wrapper = FM_QIMAGE_WRAPPER(self);
  // placement delete
  wrapper->image.~QImage();
}

GObject *fm_qimage_wrapper_new(QImage& image) {
  FmQImageWrapper *wrapper = (FmQImageWrapper*)g_object_new(FM_TYPE_QIMAGE_WRAPPER, NULL);
  wrapper->image = image;
  return (GObject*)wrapper;
}

ThumbnailLoader* ThumbnailLoader::theThumbnailLoader = NULL;

ThumbnailLoader::ThumbnailLoader() {
  ThumbnailLoaderBackend qt_backend = {
    readImageFromFile,
    readImageFromStream,
    writeImage,
    scaleImage,
    rotateImage,
    getImageWidth,
    getImageHeight,
    getImageText
  };
  fm_thumbnail_loader_set_backend(&qt_backend);
}

ThumbnailLoader::~ThumbnailLoader() {

}

GObject* ThumbnailLoader::readImageFromFile(const char* filename) {
  QImage image;
  image.load(QString(filename));
  return image.isNull() ? NULL : fm_qimage_wrapper_new(image);
}

GObject* ThumbnailLoader::readImageFromStream(GInputStream* stream, GCancellable* cancellable) {
  QByteArray data;
  data.reserve(1024 * 1024 * 4); // FIXME: get the size from the file info
  while(!g_cancellable_is_cancelled(cancellable)) {
    char buffer[4096];
    // FIXME: in the future we can create a QIODevice class based on GInputStream, so we
    // can avoid the unnecessary copies here.
    gssize size = g_input_stream_read(stream, buffer, 4096, cancellable, NULL);
    if(size == 0) // end of file
      break;
    else if(size == -1) // error
      return NULL;
    data.append(buffer, size);
  }
  QImage image;
  image.loadFromData(data);
  return image.isNull() ? NULL : fm_qimage_wrapper_new(image);
}

gboolean ThumbnailLoader::writeImage(GObject* image, const char* filename, const char* uri, const char* mtime) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(image);
  wrapper->image.setText("tEXt::Thumb::URI", uri);
  wrapper->image.setText("tEXt::Thumb::MTime", mtime);
  return (gboolean)wrapper->image.save(filename);
}

GObject* ThumbnailLoader::scaleImage(GObject* ori_pix, int new_width, int new_height) {
  qDebug("scaleImage: %d, %d", new_width, new_height);
  FmQImageWrapper* ori_wrapper = FM_QIMAGE_WRAPPER(ori_pix);
  QImage scaled = ori_wrapper->image.scaled(new_width, new_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  return scaled.isNull() ? NULL : fm_qimage_wrapper_new(scaled);
}

GObject* ThumbnailLoader::rotateImage(GObject* image, int degree) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(image);
  QImage rotated = wrapper->image.transformed(QMatrix().rotate(degree));
  return rotated.isNull() ? NULL : fm_qimage_wrapper_new(rotated);
}

int ThumbnailLoader::getImageWidth(GObject* image) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(image);
  return wrapper->image.width();
}

int ThumbnailLoader::getImageHeight(GObject* image) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(image);
  return wrapper->image.height();
}

char* ThumbnailLoader::getImageText(GObject* image, const char* key) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(image);
  QByteArray text = wrapper->image.text(key).toAscii();
  return g_memdup(text.constData(), text.length());
}

QImage ThumbnailLoader::image(FmThumbnailResult* result) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(fm_thumbnail_result_get_data(result));
  if(wrapper) {
    return wrapper->image;
  }
  return QImage();
}

