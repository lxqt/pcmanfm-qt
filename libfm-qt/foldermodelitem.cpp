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


#include "foldermodelitem.h"
#include <QFileInfo>
#include <QDebug>
#include <QProcess>
#include "bundle.h"

using namespace Fm;

FolderModelItem::FolderModelItem(FmFileInfo* _info):
  info(fm_file_info_ref(_info)) {
  displayName = QString::fromUtf8(fm_file_info_get_disp_name(info));
  // qDebug() << "probono: (1) FolderModelItem created for" << displayName;

  bool isAppDirOrBundle = checkWhetherAppDirOrBundle(_info);

  icon = IconTheme::icon(fm_file_info_get_icon(_info));

  // probono: Set some things differently for AppDir/app bundle than for normal folder
  if(isAppDirOrBundle) {

      QString path = QString(fm_path_to_str(fm_file_info_get_path(info)));
      QFileInfo fileInfo = QFileInfo(path);
      QString nameWithoutSuffix = QFileInfo(fileInfo.completeBaseName()).fileName();

      qDebug("probono: AppDir/app bundle detected xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx " + path.toUtf8());

      qDebug("probono: Set different icon for AppDir/app bundle");
      icon = getIconForBundle(info);

      // probono: Set display name
      fm_file_info_set_disp_name(_info, nameWithoutSuffix.toUtf8()); // probono: Remove the suffix from display name
      qDebug("probono: TODO: Set the proper display name for AppDir based on Name= entries in desktop file. Similar to what happens when desktop files are displayed");

      qDebug("probono: TODO: Submit it to some Launch Services like database?");

  }

  thumbnails.reserve(2);

}

FolderModelItem::FolderModelItem(const FolderModelItem& other) {
  info = other.info ? fm_file_info_ref(other.info) : NULL;
  displayName = QString::fromUtf8(fm_file_info_get_disp_name(info));

  if (displayName == "File System") {
    displayName = "Startvolume";
  }

  QString mimetype;
  mimetype = QString::fromUtf8(fm_mime_type_get_type(fm_mime_type_ref(fm_file_info_get_mime_type(info))));
  if (mimetype == "inode/mount-point") {
    qDebug() << "probono: Get the 'Volume label' for the volume";
#ifdef __FreeBSD__
    qDebug() << "probono: Using 'fstyp -l /dev/" + displayName + "' on FreeBSD";
    // NOTE: placesmodelitem.cpp has similar code for what gets shown in the sidebar
    // NOTE: Alternatively, we could just use mountpoints that have the volume label as their name
    QProcess p;
    QString program = "fstyp";
    QStringList arguments;
    arguments << "-l" << "/dev/" + displayName;
    p.start(program, arguments);
    p.waitForFinished();
    QString result(p.readAllStandardOutput());
    result.replace("\n", "");
    result = result.trimmed();
    qDebug() << "probono: result:" << result;
    if (result.split(" ").length() == 1) {
        if (result.split(" ")[0] != "" && result.split(" ")[0] != nullptr) {
            // We got a filesystem but no volume label back, so use the filesystem
            displayName = result.split(" ")[0];
        }
    } else if (result.split(" ").length() == 2) {
        if (result.split(" ")[1] != "" && result.split(" ")[1] != nullptr) {
            // We got a filesystem and a volume label back, so use the volume label
            displayName = result.split(" ")[1];
        }
    }
#else
    qDebug() << "probono: TODO: To be implemented for this OS";
#endif
  }

  // qDebug() << "probono: (2) FolderModelItem created for" << displayName;

  icon = other.icon;
  thumbnails = other.thumbnails;
}

FolderModelItem::~FolderModelItem() {
  if(info)
    // qDebug("probono: FolderModelItem destroyed for");
    // qDebug(fm_file_info_get_disp_name(info));
    fm_file_info_unref(info);

}

// find thumbnail of the specified size
// The returned thumbnail item is temporary and short-lived
// If you need to use the struct later, copy it to your own struct to keep it.
FolderModelItem::Thumbnail* FolderModelItem::findThumbnail(int size) {

  QVector<Thumbnail>::iterator it;
  for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
    if(it->size == size) { // an image of the same size is found
      return it;
    }
  }
  if(it == thumbnails.end()) {
    Thumbnail thumbnail;
    thumbnail.status = ThumbnailNotChecked;
    thumbnail.size = size;
    thumbnails.append(thumbnail);
  }
  return &thumbnails.back();
}

// remove cached thumbnail of the specified size
void FolderModelItem::removeThumbnail(int size) {
  QVector<Thumbnail>::iterator it;
  for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
    if(it->size == size) { // an image of the same size is found
      thumbnails.erase(it);
      break;
    }
  }
}

#if 0
// cache the thumbnail of the specified size in the folder item
void FolderModelItem::setThumbnail(int size, QImage image) {
  QVector<Thumbnail>::iterator it;
  for(it = thumbnails.begin(); it != thumbnails.end(); ++it) {
    if(it->size == size) { // an image of the same size already exists
      it->image = image; // replace it
      it->status = ThumbnailLoaded;
      break;
    }
  }
  if(it == thumbnails.end()) { // the image is not found
    Thumbnail thumbnail;
    thumbnail.size = size;
    thumbnail.status = ThumbnailLoaded;
    thumbnail.image = image;
    thumbnails.append(thumbnail); // add a new entry
  }
}
#endif
