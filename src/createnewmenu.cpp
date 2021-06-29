/*
    Menu with entries to create new folders and files.
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

#include "createnewmenu.h"
#include "folderview.h"
#include "icontheme.h"
#include "utilities.h"

namespace Fm {

CreateNewMenu::CreateNewMenu(QWidget* dialogParent, FmPath* dirPath, QWidget* parent):
  QMenu(parent), dialogParent_(dialogParent), dirPath_(dirPath) {
  QAction* action = new QAction(QIcon::fromTheme("folder-new"), tr("Folder"), this);
  connect(action, &QAction::triggered, this, &CreateNewMenu::onCreateNewFolder);
  addAction(action);

  action = new QAction(QIcon::fromTheme("document-new"), tr("Blank File"), this);
  connect(action, &QAction::triggered, this, &CreateNewMenu::onCreateNewFile);
  addAction(action);

  // add more items to "Create New" menu from templates
  GList* templates = fm_template_list_all(fm_config->only_user_templates);
  if(templates) {
    addSeparator();
    for(GList* l = templates; l; l = l->next) {
      FmTemplate* templ = (FmTemplate*)l->data;
      /* we support directories differently */
      if(fm_template_is_directory(templ))
        continue;
      FmMimeType* mime_type = fm_template_get_mime_type(templ);
      const char* label = fm_template_get_label(templ);
      QString text = QString("%1 (%2)").arg(QString::fromUtf8(label)).arg(QString::fromUtf8(fm_mime_type_get_desc(mime_type)));
      FmIcon* icon = fm_template_get_icon(templ);
      if(!icon)
        icon = fm_mime_type_get_icon(mime_type);
      QAction* action = addAction(IconTheme::icon(icon), text);
      action->setObjectName(QString::fromUtf8(fm_template_get_name(templ, NULL)));
      connect(action, &QAction::triggered, this, &CreateNewMenu::onCreateNew);
    }
  }
}

CreateNewMenu::~CreateNewMenu() {
}

void CreateNewMenu::onCreateNewFile() {
  if(dirPath_)
    createFileOrFolder(CreateNewTextFile, dirPath_);
}

void CreateNewMenu::onCreateNewFolder() {
  if(dirPath_)
    createFileOrFolder(CreateNewFolder, dirPath_);
}

void CreateNewMenu::onCreateNew() {
  QAction* action = static_cast<QAction*>(sender());
  QByteArray name = action->objectName().toUtf8();
  GList* templates = fm_template_list_all(fm_config->only_user_templates);
  FmTemplate* templ = NULL;
  for(GList* l = templates; l; l = l->next) {
    FmTemplate* t = (FmTemplate*)l->data;
    if(name == fm_template_get_name(t, NULL)) {
      templ = t;
      break;
    }
  }
  if(templ) { // template found
    if(dirPath_)
      createFileOrFolder(CreateWithTemplate, dirPath_, templ, dialogParent_);
  }
}

} // namespace Fm
