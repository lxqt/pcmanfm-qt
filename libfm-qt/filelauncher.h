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

#include <QWidget>
#include <libfm/fm.h>

namespace Fm {

class LIBFM_QT_API FileLauncher {
public:
  FileLauncher();
  ~FileLauncher();
  static bool launch(QWidget* parent, GList* file_infos);
  static bool launch(QWidget* parent, FmFileInfoList* file_infos) {
    GList* fileList = fm_file_info_list_peek_head_link(file_infos);
    Fm::FileLauncher::launch(NULL, fileList);
  }

protected:
    static gboolean openFolder(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data, GError** err);
    // static FmFileLauncherExecAction (*exec_file)(FmFileInfo* file, gpointer user_data);
    static gboolean error(GAppLaunchContext* ctx, GError* err, FmPath* file, gpointer user_data);
    // static int (*ask)(const char* msg, char* const* btn_labels, int default_btn, gpointer user_data);  

private:
  static FmFileLauncher funcs;
};

}

#endif // FM_FILELAUNCHER_H
