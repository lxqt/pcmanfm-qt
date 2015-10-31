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
bool ThumbnailLoader::localFilesOnly_ = true;
int ThumbnailLoader::maxThumbnailFileSize_ = 0;

ThumbnailLoader::ThumbnailLoader() {
  // apply the settings to libfm
  fm_config->thumbnail_local = localFilesOnly_;
  fm_config->thumbnail_max = maxThumbnailFileSize_;

  FmThumbnailLoaderBackend qt_backend = {
    readImageFromFile,
    readImageFromStream,
    writeImage,
    scaleImage,
    rotateImage,
    getImageWidth,
    getImageHeight,
    getImageText,
    setImageText
  };
  gboolean success = fm_thumbnail_loader_set_backend(&qt_backend);
}

ThumbnailLoader::~ThumbnailLoader() {

}

GObject* ThumbnailLoader::readImageFromFile(const char* filename) {
  QImage image;
  image.load(QString(filename));
  // qDebug("readImageFromFile: %s, %d", filename, image.isNull());
  return image.isNull() ? NULL : fm_qimage_wrapper_new(image);
}

GObject* ThumbnailLoader::readImageFromStream(GInputStream* stream, guint64 len, GCancellable* cancellable) {
  // qDebug("readImageFromStream: %p, %llu", stream, len);
  // FIXME: should we set a limit here? Otherwise if len is too large, we can run out of memory.
  unsigned char* buffer = new unsigned char[len]; // allocate enough buffer
  unsigned char* pbuffer = buffer;
  int totalReadSize = 0;
  while(!g_cancellable_is_cancelled(cancellable) && totalReadSize < len) {
    int bytesToRead = totalReadSize + 4096 > len ? len - totalReadSize : 4096;
    gssize readSize = g_input_stream_read(stream, pbuffer, bytesToRead, cancellable, NULL);
    if(readSize == 0) // end of file
      break;
    else if(readSize == -1) // error
      return NULL;
    totalReadSize += readSize;
    pbuffer += readSize;
  }
  QImage image;
  image.loadFromData(buffer, totalReadSize);
  delete []buffer;
  return image.isNull() ? NULL : fm_qimage_wrapper_new(image);
}

gboolean ThumbnailLoader::writeImage(GObject* image, const char* filename) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(image);
  if(wrapper == NULL || wrapper->image.isNull())
    return FALSE;
  return (gboolean)wrapper->image.save(filename, "PNG");
}

GObject* ThumbnailLoader::scaleImage(GObject* ori_pix, int new_width, int new_height) {
  // qDebug("scaleImage: %d, %d", new_width, new_height);
  FmQImageWrapper* ori_wrapper = FM_QIMAGE_WRAPPER(ori_pix);
  QImage scaled = ori_wrapper->image.scaled(new_width, new_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  return scaled.isNull() ? NULL : fm_qimage_wrapper_new(scaled);
}

GObject* ThumbnailLoader::rotateImage(GObject* image, int degree) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(image);
  // degree values are 0, 90, 180, and 270 counterclockwise.
  // In Qt, QMatrix does rotation counterclockwise as well.
  // However, because the y axis of widget coordinate system is downward,
  // the real effect of the coordinate transformation becomes clockwise rotation.
  // So we need to use (360 - degree) here.
  // Quote from QMatrix API doc:
  // Note that if you apply a QMatrix to a point defined in widget
  // coordinates, the direction of the rotation will be clockwise because
  // the y-axis points downwards.
  QImage rotated = wrapper->image.transformed(QMatrix().rotate(360 - degree));
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
  QByteArray text = wrapper->image.text(key).toLatin1();
  return (char*)g_memdup(text.constData(), text.length());
}

gboolean ThumbnailLoader::setImageText(GObject* image, const char* key, const char* val) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(image);
  // NOTE: we might receive image=NULL sometimes with older versions of libfm.
  if(Q_LIKELY(wrapper != NULL)) {
    wrapper->image.setText(key, val);
  }
  return TRUE;
}

QImage ThumbnailLoader::image(FmThumbnailLoader* result) {
  FmQImageWrapper* wrapper = FM_QIMAGE_WRAPPER(fm_thumbnail_loader_get_data(result));
  if(wrapper) {
    return wrapper->image;
  }
  return QImage();
}

