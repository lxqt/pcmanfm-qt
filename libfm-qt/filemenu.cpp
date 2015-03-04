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


#include "filemenu.h"
#include "icontheme.h"
#include "filepropsdialog.h"
#include "utilities.h"
#include "fileoperation.h"
#include "filelauncher.h"
#include "appchooserdialog.h"
#ifdef CUSTOM_ACTIONS
#include <libfm/fm-actions.h>
#endif
#include <QMessageBox>
#include <QDebug>
#include "filemenu_p.h"

namespace Fm {

FileMenu::FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, QWidget* parent):
  QMenu(parent),
  fileLauncher_(NULL) {
  createMenu(files, info, cwd);
}

FileMenu::FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, const QString& title, QWidget* parent):
  QMenu(title, parent),
  fileLauncher_(NULL),
  unTrashAction_(NULL) {
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
  useTrash_ = true;
  confirmDelete_ = true;
  files_ = fm_file_info_list_ref(files);
  info_ = info ? fm_file_info_ref(info) : NULL;
  cwd_ = cwd ? fm_path_ref(cwd) : NULL;

  FmFileInfo* first = fm_file_info_list_peek_head(files);
  FmMimeType* mime_type = fm_file_info_get_mime_type(first);
  FmPath* path = fm_file_info_get_path(first);
  // check if the files are of the same type
  sameType_ = fm_file_info_list_is_same_type(files);
  // check if the files are on the same filesystem
  sameFilesystem_ = fm_file_info_list_is_same_fs(files);
  // check if the files are all virtual
  allVirtual_ = sameFilesystem_ && fm_path_is_virtual(path);
  // check if the files are all in the trash can
  allTrash_ =  sameFilesystem_ && fm_path_is_trash(path);

  openAction_ = new QAction(QIcon::fromTheme("document-open"), tr("Open"), this);
  connect(openAction_ , &QAction::triggered, this, &FileMenu::onOpenTriggered);
  addAction(openAction_);

  openWithMenuAction_ = new QAction(tr("Open With..."), this);
  addAction(openWithMenuAction_);
  // create the "Open with..." sub menu
  QMenu* menu = new QMenu();
  openWithMenuAction_->setMenu(menu);

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
        connect(action, &QAction::triggered, this, &FileMenu::onApplicationTriggered);
        menu->addAction(action);
      }
      g_list_free(apps); /* don't unref GAppInfos now */
    }
  }
  menu->addSeparator();
  openWithAction_ = new QAction(tr("Other Applications"), this);
  connect(openWithAction_ , &QAction::triggered, this, &FileMenu::onOpenWithTriggered);
  menu->addAction(openWithAction_);

  separator1_ = addSeparator();

  if(allTrash_) { // all selected files are in trash:///
    bool can_restore = true;
    /* only immediate children of trash:/// can be restored. */
    for(GList* l = fm_file_info_list_peek_head_link(files_); l; l=l->next) {
        FmPath *trash_path = fm_file_info_get_path(FM_FILE_INFO(l->data));
        if(!fm_path_get_parent(trash_path) ||
           !fm_path_is_trash_root(fm_path_get_parent(trash_path))) {
            can_restore = false;
            break;
        }
    }
    if(can_restore) {
      unTrashAction_ = new QAction(tr("&Restore"), this);
      connect(unTrashAction_, &QAction::triggered, this, &FileMenu::onUnTrashTriggered);
      addAction(unTrashAction_);
    }
  }
  else { // ordinary files
    cutAction_ = new QAction(QIcon::fromTheme("edit-cut"), tr("Cut"), this);
    connect(cutAction_, &QAction::triggered, this, &FileMenu::onCutTriggered);
    addAction(cutAction_);

    copyAction_ = new QAction(QIcon::fromTheme("edit-copy"), tr("Copy"), this);
    connect(copyAction_, &QAction::triggered, this, &FileMenu::onCopyTriggered);
    addAction(copyAction_);

    pasteAction_ = new QAction(QIcon::fromTheme("edit-paste"), tr("Paste"), this);
    connect(pasteAction_, &QAction::triggered, this, &FileMenu::onPasteTriggered);
    addAction(pasteAction_);

    deleteAction_ = new QAction(QIcon::fromTheme("user-trash"), tr("&Move to Trash"), this);
    connect(deleteAction_, &QAction::triggered, this, &FileMenu::onDeleteTriggered);
    addAction(deleteAction_);

    renameAction_ = new QAction(tr("Rename"), this);
    connect(renameAction_, &QAction::triggered, this, &FileMenu::onRenameTriggered);
    addAction(renameAction_);
  }

#ifdef CUSTOM_ACTIONS
  // DES-EMA custom actions integration
  GList* files_list = fm_file_info_list_peek_head_link(files);
  GList* items = fm_get_actions_for_files(files_list);
  if(items) {
    GList* l;
    for(l=items; l; l=l->next) {
      FmFileActionItem* item = FM_FILE_ACTION_ITEM(l->data);
      addCustomActionItem(this, item);
    }
  } 
  g_list_foreach(items, (GFunc)fm_file_action_item_unref, NULL);
  g_list_free(items);
#endif
  // archiver integration
  // FIXME: we need to modify upstream libfm to include some Qt-based archiver programs.
  if(!allVirtual_) {
    if(sameType_) {
      FmArchiver* archiver = fm_archiver_get_default();
      if(archiver) {
        if(fm_archiver_is_mime_type_supported(archiver, fm_mime_type_get_type(mime_type))) {
          if(cwd_ && archiver->extract_to_cmd) {
            QAction* action = new QAction(tr("Extract to..."), this);
            connect(action, &QAction::triggered, this, &FileMenu::onExtract);
            addAction(action);
          }
          if(archiver->extract_cmd) {
            QAction* action = new QAction(tr("Extract Here"), this);
            connect(action, &QAction::triggered, this, &FileMenu::onExtractHere);
            addAction(action);
          }
        }
        else {
          QAction* action = new QAction(tr("Compress"), this);
          connect(action, &QAction::triggered, this, &FileMenu::onCompress);
          addAction(action);
        }
      }
    }
  }

  separator2_ = addSeparator();

  propertiesAction_ = new QAction(QIcon::fromTheme("document-properties"), tr("Properties"), this);
  connect(propertiesAction_, &QAction::triggered, this, &FileMenu::onFilePropertiesTriggered);
  addAction(propertiesAction_);
}

#ifdef CUSTOM_ACTIONS
void FileMenu::addCustomActionItem(QMenu* menu, FmFileActionItem* item) {
  if(!item) { // separator
    addSeparator();
    return;
  }

  // this action is not for context menu
  if(fm_file_action_item_is_action(item) && !(fm_file_action_item_get_target(item) & FM_FILE_ACTION_TARGET_CONTEXT))
      return;

  CustomAction* action = new CustomAction(item, menu);
  menu->addAction(action);
  if(fm_file_action_item_is_menu(item)) {
    GList* subitems = fm_file_action_item_get_sub_items(item);
    for(GList* l = subitems; l; l = l->next) {
      FmFileActionItem* subitem = FM_FILE_ACTION_ITEM(l->data);
      QMenu* submenu = new QMenu(menu);
      addCustomActionItem(submenu, subitem);
      action->setMenu(submenu);
    }
  }
  else if(fm_file_action_item_is_action(item)) {
    connect(action, &QAction::triggered, this, &FileMenu::onCustomActionTrigerred);
  }
}
#endif

void FileMenu::onOpenTriggered() {
  if(fileLauncher_) {
    fileLauncher_->launchFiles(NULL, files_);
  }
  else { // use the default launcher
    Fm::FileLauncher launcher;
    launcher.launchFiles(NULL, files_);
  }
}

void FileMenu::onOpenWithTriggered() {
  AppChooserDialog dlg(NULL);
  if(sameType_) {
    dlg.setMimeType(fm_file_info_get_mime_type(info_));
  }
  else { // we can only set the selected app as default if all files are of the same type
    dlg.setCanSetDefault(false);
  }
  
  if(execModelessDialog(&dlg) == QDialog::Accepted) {
    GAppInfo* app = dlg.selectedApp();
    if(app) {
      openFilesWithApp(app);
      g_object_unref(app);
    }
  }
}

void FileMenu::openFilesWithApp(GAppInfo* app) {
  FmPathList* paths = fm_path_list_new_from_file_info_list(files_);
  GList* uris = NULL;
  for(GList* l = fm_path_list_peek_head_link(paths); l; l = l->next) {
    FmPath* path = FM_PATH(l->data);
    char* uri = fm_path_to_uri(path);
    uris = g_list_prepend(uris, uri);
  }
  fm_path_list_unref(paths);
  fm_app_info_launch_uris(app, uris, NULL, NULL);
  g_list_foreach(uris, (GFunc)g_free, NULL);
  g_list_free(uris);  
}

void FileMenu::onApplicationTriggered() {
  AppInfoAction* action = static_cast<AppInfoAction*>(sender());
  openFilesWithApp(action->appInfo());
}

#ifdef CUSTOM_ACTIONS
void FileMenu::onCustomActionTrigerred() {
  CustomAction* action = static_cast<CustomAction*>(sender());
  FmFileActionItem* item = action->item();

  GList* files = fm_file_info_list_peek_head_link(files_);
  char* output = NULL;
  /* g_debug("item: %s is activated, id:%s", fm_file_action_item_get_name(item),
      fm_file_action_item_get_id(item)); */
  fm_file_action_item_launch(item, NULL, files, &output);
  if(output) {
    QMessageBox::information(this, tr("Output"), QString::fromUtf8(output));
    g_free(output);
  }
}
#endif

void FileMenu::onFilePropertiesTriggered() {
  FilePropsDialog::showForFiles(files_);
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
  if(useTrash_)
    FileOperation::trashFiles(paths, confirmDelete_);
  else
    FileOperation::deleteFiles(paths, confirmDelete_);
  fm_path_list_unref(paths);
}

void FileMenu::onUnTrashTriggered() {
  FmPathList* paths = fm_path_list_new_from_file_info_list(files_);
  FileOperation::unTrashFiles(paths);
}

void FileMenu::onPasteTriggered() {
  Fm::pasteFilesFromClipboard(cwd_);
}

void FileMenu::onRenameTriggered() {
  for(GList* l = fm_file_info_list_peek_head_link(files_); l; l = l->next) {
    FmFileInfo* info = FM_FILE_INFO(l->data);
    Fm::renameFile(info, NULL);
  }
}

void FileMenu::setUseTrash(bool trash) {
  if(useTrash_ != trash) {
    useTrash_ = trash;
    deleteAction_->setText(useTrash_ ? tr("&Move to Trash") : tr("&Delete"));
    deleteAction_->setIcon(useTrash_ ? QIcon::fromTheme("user-trash") : QIcon::fromTheme("edit-delete"));
  }
}

void FileMenu::onCompress() {
  FmArchiver* archiver = fm_archiver_get_default();
  if(archiver) {
    FmPathList* paths = fm_path_list_new_from_file_info_list(files_);
    fm_archiver_create_archive(archiver, NULL, paths);
    fm_path_list_unref(paths);
  }
}

void FileMenu::onExtract() {
  FmArchiver* archiver = fm_archiver_get_default();
  if(archiver) {
    FmPathList* paths = fm_path_list_new_from_file_info_list(files_);
    fm_archiver_extract_archives(archiver, NULL, paths);
    fm_path_list_unref(paths);
  }
}

void FileMenu::onExtractHere() {
  FmArchiver* archiver = fm_archiver_get_default();
  if(archiver) {
    FmPathList* paths = fm_path_list_new_from_file_info_list(files_);
    fm_archiver_extract_archives_to(archiver, NULL, paths, cwd_);
    fm_path_list_unref(paths);
  }
}

} // namespace Fm
