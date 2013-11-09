/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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


#ifndef FM_UTILITIES_H
#define FM_UTILITIES_H

#include "libfmqtglobals.h"
#include <QClipboard>
#include <QUrl>
#include <QList>
#include <libfm/fm.h>
#include <sys/types.h>

namespace Fm {

LIBFM_QT_API FmPathList* pathListFromQUrls(QList<QUrl> urls);

LIBFM_QT_API void pasteFilesFromClipboard(FmPath* destPath, QWidget* parent = 0);

LIBFM_QT_API void copyFilesToClipboard(FmPathList* files);

LIBFM_QT_API void cutFilesToClipboard(FmPathList* files);

LIBFM_QT_API void renameFile(FmPath* file, QWidget* parent = 0);

enum CreateFileType {
  CreateNewFolder,
  CreateNewTextFile,
  CreateWithTemplate
};

LIBFM_QT_API void createFile(CreateFileType type, FmPath* parentDir, FmPath* templateFile = NULL, QWidget* parent = 0);

LIBFM_QT_API uid_t uidFromName(QString name);

LIBFM_QT_API QString uidToName(uid_t uid);

LIBFM_QT_API gid_t gidFromName(QString name);

LIBFM_QT_API QString gidToName(gid_t gid);

}

#endif // FM_UTILITIES_H
