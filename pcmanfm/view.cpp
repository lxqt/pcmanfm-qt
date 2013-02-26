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


#include "view.h"
#include "filemenu.h"
#include "foldermenu.h"
#include "filelauncher.h"

using namespace PCManFM;

View::View(Fm::FolderView::ViewMode _mode, QWidget* parent):
  Fm::FolderView(_mode, parent) {
  connect(this, SIGNAL(clicked(int,FmFileInfo*)), SLOT(onFileClicked(int,FmFileInfo*)));
}

View::~View() {

}

void View::onPopupMenuHide() {
  Fm::FileMenu* menu = static_cast<Fm::FileMenu*>(sender());
  //delete the menu;
  menu->deleteLater();
}

void View::onFileClicked(int type, FmFileInfo* fileInfo) {
  if(type == ActivatedClick) {
    if(fm_file_info_is_dir(fileInfo)) {
      Q_EMIT openDirRequested(fm_file_info_get_path(fileInfo), OpenInCurrentView);
    }
    else {
      GList* files = g_list_append(NULL, fileInfo);
      Fm::FileLauncher::launch(NULL, files);
      g_list_free(files);
    }
  }
  else if(type == ContextMenuClick) {
    FmPath* folderPath = path();
    QMenu* menu;
    if(fileInfo) {
      // show context menu
      FmFileInfoList* files = selectedFiles();
      menu = new Fm::FileMenu(files, fileInfo, folderPath);
      prepareFileMenu(menu);
      fm_file_info_list_unref(files);
    }
    else {
      FmFolder* _folder = folder();
      FmFileInfo* info =fm_folder_get_info(_folder);
      menu = new Fm::FolderMenu(this);
      prepareFolderMenu(menu);
      // TODO: create popup menu for the folder itself
    }
    menu->popup(QCursor::pos());
    connect(menu, SIGNAL(aboutToHide()),SLOT(onPopupMenuHide()));
  }
}

void View::prepareFileMenu(Fm::FileMenu* menu) {

}

void View::prepareFolderMenu(QMenu* menu) {

}

#include "view.moc"
