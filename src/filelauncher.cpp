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
#include <QDebug>
#include <QProcess>
#include <QStandardPaths>
#include "execfiledialog_p.h"
#include "appchooserdialog.h"
#include "utilities.h"
#include "bundle.h"

using namespace Fm;

FmFileLauncher FileLauncher::funcs = {
    FileLauncher::_getApp,
    /* gboolean (*before_open)(GAppLaunchContext* ctx, GList* folder_infos, gpointer user_data); */
    (FmLaunchFolderFunc)FileLauncher::_openFolder,
    FileLauncher::_execFile,
    FileLauncher::_error,
    FileLauncher::_ask
};

FileLauncher::FileLauncher():
    quickExec_(false) {
    qDebug() << "probono: FileLauncher created";
}

FileLauncher::~FileLauncher() {
    qDebug() << "probono: FileLauncher freed";
}

//static
bool FileLauncher::launchFiles(QWidget* parent, GList* file_infos, bool show_contents) {
    qDebug() << "probono: FileLauncher::launchFiles called";
    qDebug() << "probono: Determining whether it is an AppDir/.app bundle";
    // probono: This gets invoked when an icon is double clicked or "Open" is selected from the context menu
    // but not if "Open with..." is selected from the context menu
    // GAppLaunchContext is a concept from
    // GIO - GLib Input, Output and Streaming Library
    // by Red Hat, Inc.
    // https://developer.gnome.org/gio/stable/GAppInfo.html
    // probono: Is there a way to get rid of any remnants of Red Hat and Gnome? This would mean replacing libfm and glib
    // probono: file_infos is a GList of FileInfos. For each of the FileInfos we need to get the path and from that we need to determine whether we have an AppDir/.app bundle and if so, take action
    // probono: Maybe instead of using fm_launch_files we should use fm_launch_desktop_entry to get things like "pin in Dock" which possibly only works when an application was launched through a desktop file?
    //
    // probono: Look at the implementations of fm_launch_files and fm_launch_desktop_entry to see what they are doing internally, and do similar things for AppDir/.app bundle
    // probono: Interestingly, for the actual launching they call back to this file
    // probono: See https://github.com/lxde/libfm/blob/master/src/base/fm-file-launcher.c

    FmAppLaunchContext* context = fm_app_launch_context_new_for_widget(parent);
    // Since fm_launch_files needs all items to be opened in multiple tabs at once, we need
    // to construct a list that contains those that are not bundles
    GList* itemsToBeLaunched = NULL;
    for(GList* l = file_infos; l; l = l->next) {
        FmFileInfo* info = FM_FILE_INFO(l->data);

            bool isAppDirOrBundle = checkWhetherAppDirOrBundle(info);
            if(isAppDirOrBundle == false or (show_contents == true)) {
                qDebug() << "probono: Not an .AppDir or .app bundle. TODO: Make it possible to use the 'launch' command for those, too";
                // probono: URLs like network://, sftp:// and so on will continue to be handled like this in any case since they need GIO,
                // but documents, non-bundle executables etc. could all be handled by 'launch' if we make 'launch' understand them
                itemsToBeLaunched = g_list_append(itemsToBeLaunched, l->data);
            } else {
                QString launchableExecutable = getLaunchableExecutable(info);
                if(QStandardPaths::findExecutable("launch") != "") {
                    qDebug() << "probono: Launching using the 'launch' command";
                    QProcess::startDetached("launch", {launchableExecutable});
                } else {
                    qDebug() << "probono: The 'launch' command is not available on the $PATH, otherwise it would be used";
                    qDebug() << "probono: Construct FmFileInfo* for" << launchableExecutable << "and add it to itemsToBeLaunched";
                    FmFileInfo* launchableExecutableFileInfo = fm_file_info_new_from_native_file(nullptr, launchableExecutable.toUtf8(),nullptr);
                    itemsToBeLaunched = g_list_append(itemsToBeLaunched, launchableExecutableFileInfo);
                }
            }

    }

    // probono: Unlike PcManFM-Qt, we don't want that multiple selected files
    // get opened in tabs. Instead, we want each to be opened inside its own window.
    bool ret = true;
    for(GList* l = itemsToBeLaunched; l; l = l->next) {
        GList* itemToBeLaunched = NULL;
        itemToBeLaunched = g_list_append(itemToBeLaunched, l->data);
        bool result = fm_launch_files(nullptr, itemToBeLaunched, &funcs, this);
        if (result == false) {
            ret = false;
        }
        g_list_free(itemToBeLaunched);
    }

    g_list_free(itemsToBeLaunched);
    g_object_unref(context);
    return ret;
}

bool FileLauncher::launchPaths(QWidget* parent, GList* paths) {
    qDebug() << "probono: FileLauncher::launchPaths called on " << paths;
    FmAppLaunchContext* context = fm_app_launch_context_new_for_widget(parent);
    bool ret = fm_launch_paths(G_APP_LAUNCH_CONTEXT(context), paths, &funcs, this);
    g_object_unref(context);
    return ret;
}

GAppInfo* FileLauncher::getApp(GList* file_infos, FmMimeType* mime_type, GError** err) {
    AppChooserDialog dlg(NULL);
    qDebug() << "probono: FileLauncher::getApp called on " << file_infos;
    if(mime_type)
        dlg.setMimeType(mime_type);
    else
        dlg.setCanSetDefault(false);
    // FIXME: show error properly?
    if(execModelessDialog(&dlg) == QDialog::Accepted) {
        return dlg.selectedApp();
    }
    return NULL;
}

bool FileLauncher::openFolder(GAppLaunchContext* ctx, GList* folder_infos, GError** err) {
    qDebug() << "probono: FileLauncher::openFolder called";
    for(GList* l = folder_infos; l; l = l->next) {
        FmFileInfo* fi = FM_FILE_INFO(l->data);
        qDebug() << "  folder:" << QString::fromUtf8(fm_file_info_get_disp_name(fi));
    }
    return false;
}

FmFileLauncherExecAction FileLauncher::execFile(FmFileInfo* file) {
    qDebug() << "probono: FileLauncher::execFile called";
    qDebug() << "probono: TODO: check if execute bit is set and if not ask the user whether to set it";

    if (quickExec_) {
        /* SF bug#838: open terminal for each script may be just a waste.
       User should open a terminal and start the script there
       in case if user wants to see the script output anyway.
    if (fm_file_info_is_text(file))
        return FM_FILE_LAUNCHER_EXEC_IN_TERMINAL; */
        return FM_FILE_LAUNCHER_EXEC;
    }

    FmFileLauncherExecAction res = FM_FILE_LAUNCHER_EXEC_CANCEL;
    ExecFileDialog dlg(file);
    if(execModelessDialog(&dlg) == QDialog::Accepted) {
        res = dlg.result();
    }
    return res;
}

int FileLauncher::ask(const char* msg, char* const* btn_labels, int default_btn) {
    qDebug() << "probono: FileLauncher::ask called";
    /* FIXME: set default button properly */
    // return fm_askv(data->parent, NULL, msg, btn_labels);
    return -1;
}

bool FileLauncher::error(GAppLaunchContext* ctx, GError* err, FmPath* path) {
    qDebug() << "probono: FileLauncher::error called";
    /* ask for mount if trying to launch unmounted path */
    if(err->domain == G_IO_ERROR) {
        if(path && err->code == G_IO_ERROR_NOT_MOUNTED) {
            //if(fm_mount_path(data->parent, path, TRUE))
            //  return FALSE; /* ask to retry */
        }
        else if(err->code == G_IO_ERROR_FAILED_HANDLED)
            return true; /* don't show error message */
    }
    QMessageBox dlg(QMessageBox::Critical, QObject::tr("Error"), QString::fromUtf8(err->message), QMessageBox::Ok);
    execModelessDialog(&dlg);
    return true;
}

