/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

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


#include "filemenu.h"
#include "icontheme.h"
#include "filepropsdialog.h"
#include "utilities.h"
#include "fileoperation.h"
#include "filelauncher.h"

using namespace Fm;

class AppInfoAction : public QAction {

public:
  explicit AppInfoAction(GAppInfo* app, QObject* parent = 0):
    QAction(QString::fromUtf8(g_app_info_get_name(app)), parent),
    appInfo_(G_APP_INFO(g_object_ref(app))) {
    setToolTip(QString::fromUtf8(g_app_info_get_description(app)));
    GIcon* gicon = g_app_info_get_icon(app);
    QIcon icon = IconTheme::icon(gicon);
    setIcon(icon);
  }

  virtual ~AppInfoAction() {
    if(appInfo_)
      g_object_unref(appInfo_);
  }

  GAppInfo* appInfo() const {
    return appInfo_;
  }

private:
  GAppInfo* appInfo_;
};


FileMenu::FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, QWidget* parent): QMenu(parent) {
  createMenu(files, info, cwd);
}

FileMenu::FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, const QString& title, QWidget* parent): QMenu(title, parent) {
  createMenu(files, info, cwd);
}

FileMenu::~FileMenu() {
  if(files_)
    fm_file_info_list_unref(files_);
  if(info_)
    fm_file_info_unref(info_);
  if(cwd_)
    fm_path_unref(cwd_);
}

void FileMenu::createMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd) {
  files_ = fm_file_info_list_ref(files);
  info_ = info ? fm_file_info_ref(info) : NULL;
  cwd_ = cwd ? fm_path_ref(cwd) : NULL;

  FmFileInfo* first = fm_file_info_list_peek_head(files);
  FmMimeType* mime_type = fm_file_info_get_mime_type(first);
  FmPath* path = fm_file_info_get_path(first);
  // check if the files are of the same type
  bool sameType_ = fm_file_info_list_is_same_type(files);
  // check if the files are on the same filesystem
  bool sameFilesystem_ = fm_file_info_list_is_same_fs(files);
  // check if the files are all virtual
  bool allVirtual_ = sameFilesystem_ && fm_path_is_virtual(path);
  // check if the files are all in the trash can
  bool allTrash_ =  sameFilesystem_ && fm_path_is_trash(path);

  openAction_ = new QAction(QIcon::fromTheme("document-open"), tr("Open"), this);
  connect(openAction_ , SIGNAL(triggered(bool)), SLOT(onOpenTriggered()));
  addAction(openAction_);

  openWithAction_ = new QAction(tr("OpenWith"), this);
  addAction(openWithAction_);
  // create the "Open with..." sub menu
  QMenu* menu = new QMenu();
  openWithAction_->setMenu(menu);

  if(sameType_) { /* add specific menu items for this mime type */
    if(mime_type && !allVirtual_) { /* the file has a valid mime-type and its not virtual */
      GList* apps = g_app_info_get_all_for_type(fm_mime_type_get_type(mime_type));
      GList* l;
      for(l=apps;l;l=l->next) {
        GAppInfo* app = G_APP_INFO(l->data);
      
        // check if the command really exists
        gchar * program_path = g_find_program_in_path(g_app_info_get_executable(app));
        if (!program_path)
          continue;
        g_free(program_path);

        // create a QAction for the application.
        AppInfoAction* action = new AppInfoAction(app);
        connect(action, SIGNAL(triggered(bool)), SLOT(onApplicationTriggered()));
        menu->addAction(action);
      }
      g_list_free(apps); /* don't unref GAppInfos now */
    }
  }
  
  separator1_ = addSeparator();
  
  cutAction_ = new QAction(QIcon::fromTheme("edit-cut"), tr("Cut"), this);
  connect(cutAction_, SIGNAL(triggered(bool)), SLOT(onCutTriggered()));
  addAction(cutAction_);

  copyAction_ = new QAction(QIcon::fromTheme("edit-copy"), tr("Copy"), this);
  connect(copyAction_, SIGNAL(triggered(bool)), SLOT(onCopyTriggered()));
  addAction(copyAction_);

  pasteAction_ = new QAction(QIcon::fromTheme("edit-paste"), tr("Paste"), this);
  connect(pasteAction_, SIGNAL(triggered(bool)), SLOT(onPasteTriggered()));
  addAction(pasteAction_);

  deleteAction_ = new QAction(QIcon::fromTheme("edit-delete"), tr("Delete"), this);
  connect(deleteAction_, SIGNAL(triggered(bool)), SLOT(onDeleteTriggered()));
  addAction(deleteAction_);

  renameAction_ = new QAction(tr("Rename"), this);
  connect(renameAction_, SIGNAL(triggered(bool)), SLOT(onRenameTriggered()));
  addAction(renameAction_);

  separator2_ = addSeparator();

  propertiesAction_ = new QAction(QIcon::fromTheme("document-properties"), tr("Properties"), this);
  connect(propertiesAction_, SIGNAL(triggered(bool)), SLOT(onFilePropertiesTriggered()));
  addAction(propertiesAction_);
}

void FileMenu::onOpenTriggered() {
  Fm::FileLauncher::launch(NULL, files_);
}

void FileMenu::onApplicationTriggered() {
  AppInfoAction* action = static_cast<AppInfoAction*>(sender());
  GAppInfo* appInfo = action->appInfo();
  FmPathList* paths = fm_path_list_new_from_file_info_list(files_);
  GList* uris = NULL;
  for(GList* l = fm_path_list_peek_head_link(paths); l; l = l->next) {
    FmPath* path = FM_PATH(l->data);
    char* uri = fm_path_to_uri(path);
    uris = g_list_prepend(uris, uri);
  }
  fm_path_list_unref(paths);
  fm_app_info_launch_uris(appInfo, uris, NULL, NULL);
  g_list_foreach(uris, (GFunc)g_free, NULL);
  g_list_free(uris);  
}

void FileMenu::onFilePropertiesTriggered() {
  FilePropsDialog* dlg = new FilePropsDialog(files_);
  dlg->show();
}

void FileMenu::onCopyTriggered() {
  FmPathList* paths = fm_path_list_new_from_file_info_list(files_);
  Fm::copyFilesToClipboard(paths);
  fm_path_list_unref(paths);
}

void FileMenu::onCutTriggered() {
  FmPathList* paths = fm_path_list_new_from_file_info_list(files_);
  Fm::cutFilesToClipboard(paths);
  fm_path_list_unref(paths);
}

void FileMenu::onDeleteTriggered() {
  FmPathList* paths = fm_path_list_new_from_file_info_list(files_);
  FileOperation::deleteFiles(paths);
  fm_path_list_unref(paths);
}

void FileMenu::onPasteTriggered() {
  Fm::pasteFilesFromClipboard(cwd_);
}

void FileMenu::onRenameTriggered() {

}

#include "filemenu.moc"