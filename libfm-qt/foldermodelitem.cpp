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
# include <QFileInfo>
#include "bundle.h"

using namespace Fm;

FolderModelItem::FolderModelItem(FmFileInfo* _info):
  info(fm_file_info_ref(_info)) {
  displayName = QString::fromUtf8(fm_file_info_get_disp_name(info));
  // qDebug("probono: FolderModelItem created for");
  // qDebug(fm_file_info_get_disp_name(info));


  bool isAppDirOrBundle = checkWhetherAppDirOrBundle(_info);

  icon = IconTheme::icon(fm_file_info_get_icon(_info));

  // probono: Set some things differently for AppDir/app bundle than for normal folder
  if(isAppDirOrBundle) {


      QString path = QString(fm_path_to_str(fm_file_info_get_path(info)));
      QFileInfo fileInfo = QFileInfo(path);
      QString nameWithoutSuffix = QFileInfo(fileInfo.completeBaseName()).fileName();

       qDebug("probono: AppDir/app bundle detected xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx " + path.toUtf8());

      qDebug("probono: Set different icon for AppDir/app bundle");
      icon = QIcon::fromTheme("do"); // probono: In the elementary theme, this is a folder with an executable icon inside it; TODO: Find more suitable one

      // probono: GNUstep .app bundle
      // http://www.gnustep.org/resources/documentation/Developer/Gui/ProgrammingManual/AppKit_1.html says:
      // To determine the icon for a folder, if the folder has a ’.app’, ’.debug’ or ’.profile’ extension - examine the Info.plist file
      // for an ’NSIcon’ value and try to use that. If there is no value specified - try foo.app/foo.tiff’ or ’foo.app/.dir.tiff’
      // TODO: Implement plist parsing. For now we just check for foo.app/Resources/foo.tiff’ and ’foo.app/.dir.tiff’
      // Actually there may be foo.app/Resources/foo.desktop files which point to Icon= and we could use that; just be sure to convert the absolute path there into a relative one?
      QFile tiffFile1(path.toUtf8() + "/Resources/" + nameWithoutSuffix.toUtf8() + ".tiff");
      if (tiffFile1.exists()) {
          icon = QIcon(QFileInfo(tiffFile1).canonicalFilePath());
      }
      QFile tiffFile2(path.toUtf8() + "/.dir.tiff");
      if (tiffFile2.exists()) {
          icon = QIcon(QFileInfo(tiffFile2).canonicalFilePath());
      }

      // probono: ROX AppDir
      QFile dirIconFile(path.toUtf8() + "/.DirIcon");
      if (dirIconFile.exists()) {
          icon = QIcon(QFileInfo(dirIconFile).canonicalFilePath());
      }

      // probono: macOS .app bundle
      // TODO: Implement plist parsing. For now we just check for foo.app/Contents/Resources/foo.icns’
      // TODO: Need to actually use CFBundleIconFile from Info.plist instead
      QFile icnsFile(path.toUtf8() + "/Contents/Resources/" + nameWithoutSuffix.toUtf8() + ".icns");
      if (icnsFile.exists()) {
          icon = QIcon(QFileInfo(icnsFile).canonicalFilePath());
      }

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
  // qDebug("probono: FolderModelItem created for");
  // qDebug(fm_file_info_get_disp_name(info));
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
