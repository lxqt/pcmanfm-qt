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


#include "filelauncher.h"
#include "applaunchcontext.h"

using namespace Fm;

FmFileLauncher FileLauncher::funcs = {
    NULL,
    /* gboolean (*before_open)(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data); */
    (FmLaunchFolderFunc)FileLauncher::openFolder,
    // FmFileLauncherExecAction (*exec_file)(FmFileInfo* file, gpointer user_data);
    NULL,
    FileLauncher::error,
    // int (*ask)(const char* msg, char* const* btn_labels, int default_btn, gpointer user_data);  
    NULL
};

FileLauncher::FileLauncher() {

}

FileLauncher::~FileLauncher() {

}

//static
bool FileLauncher::launch(QWidget* parent, GList* file_infos) {
  FmAppLaunchContext* context = fm_app_launch_context_new_for_widget(parent);
  bool ret = fm_launch_files(G_APP_LAUNCH_CONTEXT(context), file_infos, &funcs, parent);
  g_object_unref(context);
  return ret;
}

//static
gboolean FileLauncher::openFolder(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data, GError** err) {
  return FALSE;
}

// static FmFileLauncherExecAction (*exec_file)(FmFileInfo* file, gpointer user_data);

//static
gboolean FileLauncher::error(GAppLaunchContext* ctx, GError* err, FmPath* file, gpointer user_data) {
  return TRUE;
}
  