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

  QAction* action;
  action= new QAction(QIcon::fromTheme("document-open"), tr("Open"), this);
  addAction(action);
  
  action= new QAction(tr("OpenWith"), this);
  addAction(action);
  // create the "Open with..." sub menu
  QMenu* menu = new QMenu();
  action->setMenu(menu);

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
	action = new AppInfoAction(app);
	connect(action, SIGNAL(triggered(bool)), SLOT(onApplicationTriggered(bool)));
	menu->addAction(action);
      }
      g_list_free(apps); /* don't unref GAppInfos now */
    }
  }
  
  addSeparator();
  
  action= new QAction(QIcon::fromTheme("edit-cut"), tr("Cut"), this);
  addAction(action);

  action= new QAction(QIcon::fromTheme("edit-copy"), tr("Copy"), this);
  addAction(action);

  action= new QAction(QIcon::fromTheme("edit-paste"), tr("Paste"), this);
  addAction(action);

  action= new QAction(QIcon::fromTheme("edit-delete"), tr("Delete"), this);
  addAction(action);

  action= new QAction(tr("Rename"), this);
  addAction(action);

  addSeparator();

  action= new QAction(QIcon::fromTheme("document-properties"), tr("Properties"), this);
  connect(action, SIGNAL(triggered(bool)), SLOT(onFilePropertiesTriggered(bool)));
  addAction(action);
}

void FileMenu::onApplicationTriggered(bool checked) {
  AppInfoAction* action = static_cast<AppInfoAction*>(sender());
  GAppInfo* appInfo = action->appInfo();

  // TODO: Launch the application selected
}

void FileMenu::onFilePropertiesTriggered(bool checked) {
  FilePropsDialog dlg(files_, this);
  dlg.exec();
}

#include "filemenu.moc"