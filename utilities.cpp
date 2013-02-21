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


#include "utilities.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QList>

#include "fileoperation.h"

using namespace Fm;

namespace Fm {

FmPathList* pathListFromQUrls(QList<QUrl> urls) {
  QList<QUrl>::const_iterator it;
  FmPathList* pathList = fm_path_list_new();
  for(it = urls.begin(); it != urls.end(); ++it) {
    QUrl url = *it;
    FmPath* path = fm_path_new_for_uri(url.toString().toUtf8());
    fm_path_list_push_tail(pathList, path);
    fm_path_unref(path);
  }
  return pathList;
}

void pasteFilesFromClipboard(FmPath* destPath, QWidget* parent) {
  QClipboard* clipboard = QApplication::clipboard();
  const QMimeData* data = clipboard->mimeData();
  if(data->hasUrls()) {
    FmPathList* paths = Fm::pathListFromQUrls(data->urls());
    FileOperation::copyFiles(paths, destPath, parent);
    fm_path_list_unref(paths);
  }
}

void copyFilesToClipboard(FmPathList* files) {
  
}

void cutFilesToClipboard(FmPathList* files) {
  
}

};
