/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "launcher.h"
#include "mainwindow.h"
#include "application.h"
#include <libfm-qt/core/filepath.h>

namespace PCManFM {

Launcher::Launcher(PCManFM::MainWindow* mainWindow):
    Fm::FileLauncher(),
    mainWindow_(mainWindow) {

    Application* app = static_cast<Application*>(qApp);
    setQuickExec(app->settings().quickExec());
}

Launcher::~Launcher() {

}

bool Launcher::openFolder(GAppLaunchContext* ctx, GList* folder_infos, GError** err) {
    GList* l = folder_infos;
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    Application* app = static_cast<Application*>(qApp);
    MainWindow* mainWindow = mainWindow_;
    Fm::FilePath path{fm_path_to_gfile(fm_file_info_get_path(fi)), false};
    if(!mainWindow) {
        mainWindow = new MainWindow(std::move(path));
        mainWindow->resize(app->settings().windowWidth(), app->settings().windowHeight());

        if(app->settings().windowMaximized()) {
            mainWindow->setWindowState(mainWindow->windowState() | Qt::WindowMaximized);
        }
    }
    else {
        mainWindow->chdir(std::move(path));
    }
    l = l->next;
    for(; l; l = l->next) {
        fi = FM_FILE_INFO(l->data);
        path = Fm::FilePath{fm_path_to_gfile(fm_file_info_get_path(fi)), false};
        mainWindow->addTab(std::move(path));
    }
    mainWindow->show();
    mainWindow->raise();
    return true;
}

} //namespace PCManFM
