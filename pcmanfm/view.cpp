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


#include "view.h"
#include "filemenu.h"
#include "foldermenu.h"
#include "filelauncher.h"
#include "application.h"
#include "settings.h"
#include "application.h"
#include "mainwindow.h"
#include <QAction>

using namespace PCManFM;

View::View(Fm::FolderView::ViewMode _mode, QWidget* parent):
  Fm::FolderView(_mode, parent) {

  Settings& settings = static_cast<Application*>(qApp)->settings();

  setIconSize(Fm::FolderView::IconMode, QSize(settings.bigIconSize(), settings.bigIconSize()));
  setIconSize(Fm::FolderView::CompactMode, QSize(settings.smallIconSize(), settings.smallIconSize()));
  setIconSize(Fm::FolderView::ThumbnailMode, QSize(settings.thumbnailIconSize(), settings.thumbnailIconSize()));
  setIconSize(Fm::FolderView::DetailedListMode, QSize(settings.smallIconSize(), settings.smallIconSize()));

  connect(this, SIGNAL(clicked(int, FmFileInfo*)), SLOT(onFileClicked(int, FmFileInfo*)));
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
  else if(type == MiddleClick) {
    if(fm_file_info_is_dir(fileInfo)) {
      Q_EMIT openDirRequested(fm_file_info_get_path(fileInfo), OpenInNewTab);
    }
  }
  else if(type == ContextMenuClick) {
    FmPath* folderPath = path();
    QMenu* menu;
    if(fileInfo) {
      Application* app = static_cast<Application*>(qApp);
      Settings& settings = app->settings();
      // show context menu
      FmFileInfoList* files = selectedFiles();
      Fm::FileMenu* fileMenu = new Fm::FileMenu(files, fileInfo, folderPath);
      fileMenu->setConfirmDelete(settings.confirmDelete());
      fileMenu->setUseTrash(settings.useTrash());
      prepareFileMenu(fileMenu);
      fm_file_info_list_unref(files);
      menu = fileMenu;
    }
    else {
      FmFolder* _folder = folder();
      FmFileInfo* info = fm_folder_get_info(_folder);
      Fm::FolderMenu* folderMenu = new Fm::FolderMenu(this);
      prepareFolderMenu(folderMenu);
      menu = folderMenu;
    }
    menu->popup(QCursor::pos());
    connect(menu, SIGNAL(aboutToHide()), SLOT(onPopupMenuHide()));
  }
}

void View::onNewWindow() {
  Fm::FileMenu* menu = static_cast<Fm::FileMenu*>(sender()->parent());
  // FIXME: open the files in a new window
  Application* app = static_cast<Application*>(qApp);
  app->openFolders(menu->files());
}

void View::onNewTab() {
  Fm::FileMenu* menu = static_cast<Fm::FileMenu*>(sender()->parent());
  for(GList* l = fm_file_info_list_peek_head_link(menu->files()); l; l = l->next) {
    FmFileInfo* file = FM_FILE_INFO(l->data);
    Q_EMIT openDirRequested(fm_file_info_get_path(file), OpenInNewTab);
  }
}

void View::onOpenInTerminal() {
  Application* app = static_cast<Application*>(qApp);
  Fm::FileMenu* menu = static_cast<Fm::FileMenu*>(sender()->parent());
  for(GList* l = fm_file_info_list_peek_head_link(menu->files()); l; l = l->next) {
    FmFileInfo* file = FM_FILE_INFO(l->data);
    app->openFolderInTerminal(fm_file_info_get_path(file));
  }
}

void View::onSearch() {
  
}

void View::prepareFileMenu(Fm::FileMenu* menu) {
  // add some more menu items for dirs
  bool all_native = true;
  FmFileInfoList* files = menu->files();
  for(GList* l = fm_file_info_list_peek_head_link(files); l; l = l->next) {
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    if(!fm_file_info_is_dir(fi))
      return; /* actions are valid only if all selected are directories */
    else if(!fm_file_info_is_native(fi))
      all_native = false;
  }

  // hide "Open with" for selected dirs
  menu->openWithAction()->setVisible(false);
  
  QAction* action = new QAction(QIcon::fromTheme("window-new"), tr("Open in New T&ab"), menu);
  connect(action, SIGNAL(triggered(bool)), SLOT(onNewTab()));
  menu->insertAction(menu->separator1(), action);

  action = new QAction(QIcon::fromTheme("window-new"), tr("Open in New Win&dow"), menu);
  connect(action, SIGNAL(triggered(bool)), SLOT(onNewWindow()));
  menu->insertAction(menu->separator1(), action);

  // TODO: add search
  // action = menu->addAction(_("Search"));
  if(all_native) {
    action = new QAction(QIcon::fromTheme("utilities-terminal"), tr("Open in Termina&l"), menu);
    connect(action, SIGNAL(triggered(bool)), SLOT(onOpenInTerminal()));
    menu->insertAction(menu->separator1(), action);
  }
}

void View::prepareFolderMenu(Fm::FolderMenu* menu) {

}

void View::updateFromSettings(Settings& settings) {

  setIconSize(Fm::FolderView::IconMode, QSize(settings.bigIconSize(), settings.bigIconSize()));
  setIconSize(Fm::FolderView::CompactMode, QSize(settings.smallIconSize(), settings.smallIconSize()));
  setIconSize(Fm::FolderView::ThumbnailMode, QSize(settings.thumbnailIconSize(), settings.thumbnailIconSize()));
  setIconSize(Fm::FolderView::DetailedListMode, QSize(settings.smallIconSize(), settings.smallIconSize()));

  Fm::ProxyFolderModel* proxyModel = model();
  if(proxyModel) {
    proxyModel->setShowThumbnails(settings.showThumbnails());
  }
}

