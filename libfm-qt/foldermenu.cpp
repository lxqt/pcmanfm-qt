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
#include "filepropsdialog.h"
#include "folderview.h"
#include "utilities.h"

using namespace Fm;

FolderMenu::FolderMenu(FolderView* view, QWidget* parent):
  QMenu(parent),
  view_(view) {

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

  sortAction_ = new QAction(tr("Sorting"), this);
  addAction(sortAction_);
  QMenu* sortMenu = createSortMenu();
  sortAction_->setMenu(sortMenu);

  showHiddenAction_ = new QAction(tr("Show Hidden"), this);
  addAction(showHiddenAction_);
  connect(showHiddenAction_, SIGNAL(triggered(bool)), SLOT(onShowHiddenActionTriggered(bool)));

  separator4_ = addSeparator();

  propertiesAction_ = new QAction(tr("Folder Pr&operties"), this);
  addAction(propertiesAction_);
  connect(propertiesAction_, SIGNAL(triggered(bool)), SLOT(onPropertiesActionTriggered()));
}

FolderMenu::~FolderMenu() {
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
  QActionGroup* group = new QActionGroup(sortMenu);
  group->setExclusive(true);

  QAction* actionByFileName = new QAction(tr("By File Name"), this);
  sortMenu->addAction(actionByFileName);
  actionByFileName->setCheckable(true);
  group->addAction(actionByFileName);

  QAction* actionByMTime = new QAction(tr("By Modification Time"), this);
  actionByMTime->setCheckable(true);
  sortMenu->addAction(actionByMTime);
  group->addAction(actionByMTime);
  
  QAction* actionBySize = new QAction(tr("By File Size"), this);
  actionBySize->setCheckable(true);
  sortMenu->addAction(actionBySize);
  group->addAction(actionBySize);

  QAction* actionByFileType = new QAction(tr("By File Type"), this);
  actionByFileType->setCheckable(true);
  sortMenu->addAction(actionByFileType);
  group->addAction(actionByFileType);
  
  QAction* actionByOwner = new QAction(tr("By Owner"), this);
  actionByOwner->setCheckable(true);
  sortMenu->addAction(actionByOwner);
  group->addAction(actionByOwner);
  
  sortMenu->addSeparator();

  group = new QActionGroup(sortMenu);
  group->setExclusive(true);
  QAction* actionAscending = new QAction(tr("Ascending"), this);
  actionAscending->setCheckable(true);
  sortMenu->addAction(actionAscending);
  group->addAction(actionAscending);
  
  QAction* actionDescending = new QAction(tr("Descending"), this);
  actionDescending->setCheckable(true);
  sortMenu->addAction(actionDescending);
  group->addAction(actionDescending);
  
  QAction* actionFolderFirst = new QAction(tr("Folder First"), this);
  sortMenu->addAction(actionFolderFirst);
  actionFolderFirst->setCheckable(true);

  QAction* actionCaseSensitive = new QAction(tr("Case Sensitive"), this);
  sortMenu->addAction(actionCaseSensitive);
  actionCaseSensitive->setCheckable(true);

  return sortMenu;
}

void FolderMenu::onPasteActionTriggered() {
  FmPath* folderPath = view_->path();
  if(folderPath)
    pasteFilesFromClipboard(folderPath);
}

void FolderMenu::onSelectAllActionTriggered() {
  view_->selectAll();
}

void FolderMenu::onInvertSelectionActionTriggered() {
  view_->invertSelection();
}

void FolderMenu::onSortActionTriggered() {
  ProxyFolderModel* model = view_->model();
  if(model) {
    // model->sort();
  }
}

void FolderMenu::onShowHiddenActionTriggered(bool checked) {
  ProxyFolderModel* model = view_->model();
  if(model)
    model->setShowHidden(checked);
}

void FolderMenu::onPropertiesActionTriggered() {
  FmFileInfo* folderInfo = view_->folderInfo();
  if(folderInfo)
    FilePropsDialog::showForFile(folderInfo);
}

#include "foldermenu.moc"
