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
  Fm::FileInfo fi = FM_FILE_INFO(l->data);
  Application* app = static_cast<Application*>(qApp);
  MainWindow* mainWindow = mainWindow_;
  if(!mainWindow) {
    mainWindow = new MainWindow(fi.getPath());
    mainWindow->resize(app->settings().windowWidth(), app->settings().windowHeight());

    if(app->settings().windowMaximized()) {
        mainWindow->setWindowState(mainWindow->windowState() | Qt::WindowMaximized);
    }
  }
  else
    mainWindow->chdir(fi.getPath());
  l = l->next;
  for(; l; l = l->next) {
    fi = FM_FILE_INFO(l->data);
    mainWindow->addTab(fi.getPath());
  }
  mainWindow->show();
  mainWindow->raise();
  return true;
}

} //namespace PCManFM
