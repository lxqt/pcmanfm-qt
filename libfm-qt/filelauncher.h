/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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


#ifndef FM_FILELAUNCHER_H
#define FM_FILELAUNCHER_H

#include "libfmqtglobals.h"
#include <QWidget>
#include <libfm/fm.h>

namespace Fm {

class LIBFM_QT_API FileLauncher {
public:
  FileLauncher();
  virtual ~FileLauncher();

  bool launchFiles(QWidget* parent, FmFileInfoList* file_infos) {
    GList* fileList = fm_file_info_list_peek_head_link(file_infos);
    return launchFiles(parent, fileList);
  }
  bool launchPaths(QWidget* parent, FmPathList* paths) {
    GList* pathList = fm_path_list_peek_head_link(paths);
    return launchPaths(parent, pathList);
  }

  bool launchFiles(QWidget* parent, GList* file_infos);
  bool launchPaths(QWidget* parent, GList* paths);

protected:

  virtual GAppInfo* getApp(GList* file_infos, FmMimeType* mime_type, GError** err);
  virtual bool openFolder(GAppLaunchContext* ctx, GList* folder_infos, GError** err);
  virtual FmFileLauncherExecAction execFile(FmFileInfo* file);
  virtual bool error(GAppLaunchContext* ctx, GError* err, FmPath* path);
  virtual int ask(const char* msg, char* const* btn_labels, int default_btn);

private:
  static GAppInfo* _getApp(GList* file_infos, FmMimeType* mime_type, gpointer user_data, GError** err) {
     return reinterpret_cast<FileLauncher*>(user_data)->getApp(file_infos, mime_type, err);
  }
  static gboolean _openFolder(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data, GError** err) {
    return reinterpret_cast<FileLauncher*>(user_data)->openFolder(ctx, folder_infos, err);
  }
  static FmFileLauncherExecAction _execFile(FmFileInfo* file, gpointer user_data) {
    return reinterpret_cast<FileLauncher*>(user_data)->execFile(file);
  }
  static gboolean _error(GAppLaunchContext* ctx, GError* err, FmPath* file, gpointer user_data) {
    return reinterpret_cast<FileLauncher*>(user_data)->error(ctx, err, file);
  }
  static int _ask(const char* msg, char* const* btn_labels, int default_btn, gpointer user_data) {
    return reinterpret_cast<FileLauncher*>(user_data)->ask(msg, btn_labels, default_btn);
  }

private:
  static FmFileLauncher funcs;
};

}

#endif // FM_FILELAUNCHER_H
