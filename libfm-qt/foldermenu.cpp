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


#include "foldermenu.h"

using namespace Fm;

FolderMenu::FolderMenu(FmFileInfo* folderInfo, QWidget* parent):
  QMenu(parent),
  folderInfo_(fm_file_info_ref(folderInfo)) {

  createAction_ = new QAction(tr("Create &New"), this);
  addAction(createAction_);

  QMenu* createMenu = createCreateNewMenu();
  createAction_->setMenu(createMenu);

  separator1_ = addSeparator();

  pasteAction_ = new QAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), this);
  addAction(pasteAction_);
  connect(pasteAction_, SIGNAL(triggered(bool)), SLOT(onPasteActionTriggered()));

  separator2_ = addSeparator();

  selectAllAction_ = new QAction(tr("Select &All"), this);
  addAction(selectAllAction_);
  connect(selectAllAction_, SIGNAL(triggered(bool)), SLOT(onSelectAllActionTriggered()));
  
  invertSelectionAction_ = new QAction(tr("Invert Selection"), this);
  addAction(invertSelectionAction_);
  connect(invertSelectionAction_, SIGNAL(triggered(bool)), SLOT(onInvertSelectionActionTriggered()));
  
  separator3_ = addSeparator();

  sortAction_ = new QAction(tr("Sort"), this);
  addAction(sortAction_);
  QMenu* sortMenu = createSortMenu();
  sortAction_->setMenu(sortMenu);

  showHiddenAction_ = new QAction(tr("Show Hidden"), this);
  addAction(showHiddenAction_);
  connect(showHiddenAction_, SIGNAL(triggered(bool)), SLOT(onShowHiddenActionTriggered()));

  separator4_ = addSeparator();

  propertiesAction_ = new QAction(tr("Folder Pr&operties"), this);
  addAction(propertiesAction_);
  connect(propertiesAction_, SIGNAL(triggered(bool)), SLOT(onPropertiesActionTriggered()));
}

FolderMenu::~FolderMenu() {
  if(folderInfo_)
    fm_file_info_unref(folderInfo_);
}

QMenu* FolderMenu::createCreateNewMenu() {
  QMenu* createMenu = new QMenu(this);
  QAction* action = new QAction(tr("Folder"), this);
  createMenu->addAction(action);
  
  action = new QAction(tr("File"), this);
  createMenu->addAction(action);

  // TODO: add items to "Create New" menu
  return createMenu;
}

QMenu* FolderMenu::createSortMenu() {
  QMenu* sortMenu = new QMenu(this);
  QAction* actionByFileName = new QAction(tr("By File Name"), this);
  sortMenu->addAction(actionByFileName);
  
  QAction* actionByMTime = new QAction(tr("By Modification Time"), this);
  sortMenu->addAction(actionByMTime);
  
  QAction* actionByFileType = new QAction(tr("By File Type"), this);
  sortMenu->addAction(actionByFileType);
  
  QAction* actionByOwner = new QAction(tr("By Owner"), this);
  sortMenu->addAction(actionByOwner);
  
  sortMenu->addSeparator();

  QAction* actionAscending = new QAction(tr("Ascending"), this);
  sortMenu->addAction(actionAscending);
  
  QAction* actionDescending = new QAction(tr("Descending"), this);
  sortMenu->addAction(actionDescending);

  QAction* actionFolderFirst = new QAction(tr("Folder First"), this);
  sortMenu->addAction(actionFolderFirst);
  return sortMenu;
}

void FolderMenu::onPasteActionTriggered() {
}

void FolderMenu::onSelectAllActionTriggered() {
}

void FolderMenu::onInvertSelectionActionTriggered() {
}

void FolderMenu::onSortActionTriggered() {
}

void FolderMenu::onShowHiddenActionTriggered() {
}

void FolderMenu::onPropertiesActionTriggered() {
}

#include "foldermenu.moc"
