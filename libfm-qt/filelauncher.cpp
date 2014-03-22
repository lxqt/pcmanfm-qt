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
#include <QMessageBox>
#include <QEventLoop>
#include "execfiledialog.h"

using namespace Fm;

FmFileLauncher FileLauncher::funcs = {
  FileLauncher::_getApp,
  /* gboolean (*before_open)(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data); */
  (FmLaunchFolderFunc)FileLauncher::_openFolder,
  FileLauncher::_execFile,
  FileLauncher::_error,
  FileLauncher::_ask
};

FileLauncher::FileLauncher() {

}

FileLauncher::~FileLauncher() {

}

//static
bool FileLauncher::launch(QWidget* parent, GList* file_infos) {
  FmAppLaunchContext* context = fm_app_launch_context_new_for_widget(parent);
  bool ret = fm_launch_files(G_APP_LAUNCH_CONTEXT(context), file_infos, &funcs, this);
  g_object_unref(context);
  return ret;
}

GAppInfo* FileLauncher::getApp(GList* file_infos, FmMimeType* mime_type, GError** err) {
  return NULL;
}

bool FileLauncher::openFolder(GAppLaunchContext* ctx, GList* folder_infos, GError** err) {
  return false;
}

FmFileLauncherExecAction FileLauncher::execFile(FmFileInfo* file) {
  FmFileLauncherExecAction res = FM_FILE_LAUNCHER_EXEC_CANCEL;
  // FIXME: should we make a non-modal dialog here with QEventLoop?
  ExecFileDialog dlg(file);
  if(dlg.exec() == QDialog::Accepted) {
    res = dlg.result();
  }
  return res;
}

int FileLauncher::ask(const char* msg, char* const* btn_labels, int default_btn) {
  /* FIXME: set default button properly */
  // return fm_askv(data->parent, NULL, msg, btn_labels);
  return -1;
}

bool FileLauncher::error(GAppLaunchContext* ctx, GError* err, FmPath* path) {
  /* ask for mount if trying to launch unmounted path */
  if(err->domain == G_IO_ERROR) {
    if(path && err->code == G_IO_ERROR_NOT_MOUNTED) {
      //if(fm_mount_path(data->parent, path, TRUE))
      //  return FALSE; /* ask to retry */
    }
    else if(err->code == G_IO_ERROR_FAILED_HANDLED)
      return true; /* don't show error message */
  }
  QMessageBox::critical(NULL, QObject::tr("Error"), QString::fromUtf8(err->message));
  return false;
}

