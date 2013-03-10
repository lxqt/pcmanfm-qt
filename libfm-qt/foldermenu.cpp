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


#include "foldermenu.h"
#include "filepropsdialog.h"
#include "folderview.h"
#include "utilities.h"
#include <cstring> // for memset

using namespace Fm;

FolderMenu::FolderMenu(FolderView* view, QWidget* parent):
  QMenu(parent),
  view_(view) {

  ProxyFolderModel* model = view_->model();

  createAction_ = new QAction(tr("Create &New"), this);
  addAction(createAction_);

  createCreateNewMenu();
  createAction_->setMenu(createNewMenu_);

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
  createSortMenu();
  sortAction_->setMenu(sortMenu_);

  showHiddenAction_ = new QAction(tr("Show Hidden"), this);
  addAction(showHiddenAction_);
  showHiddenAction_->setCheckable(true);
  showHiddenAction_->setChecked(model->showHidden());
  connect(showHiddenAction_, SIGNAL(triggered(bool)), SLOT(onShowHiddenActionTriggered(bool)));

  separator4_ = addSeparator();

  propertiesAction_ = new QAction(tr("Folder Pr&operties"), this);
  addAction(propertiesAction_);
  connect(propertiesAction_, SIGNAL(triggered(bool)), SLOT(onPropertiesActionTriggered()));
}

FolderMenu::~FolderMenu() {
}

void FolderMenu::createCreateNewMenu() {
  QMenu* createMenu = new QMenu(this);
  QAction* action = new QAction(tr("Folder"), this);
  createMenu->addAction(action);
  
  action = new QAction(tr("File"), this);
  createMenu->addAction(action);

  // TODO: add more items to "Create New" menu
  createNewMenu_ = createMenu;
}

void FolderMenu::addSortMenuItem(QString title, int id) {
  QAction* action = new QAction(title, this);
  sortMenu_->addAction(action);
  action->setCheckable(true);
  sortActionGroup_->addAction(action);
  connect(action, SIGNAL(triggered(bool)), SLOT(onSortActionTriggered(bool)));
  sortActions_[id] = action;
}

void FolderMenu::createSortMenu() {
  ProxyFolderModel* model = view_->model();

  sortMenu_ = new QMenu(this);
  sortActionGroup_ = new QActionGroup(sortMenu_);
  sortActionGroup_->setExclusive(true);

  std::memset(sortActions_, 0, sizeof(sortActions_));

  addSortMenuItem(tr("By File Name"), FolderModel::ColumnFileName);
  addSortMenuItem(tr("By Modification Time"), FolderModel::ColumnFileMTime);
  addSortMenuItem(tr("By File Size"), FolderModel::ColumnFileSize);
  addSortMenuItem(tr("By File Type"), FolderModel::ColumnFileType);
  addSortMenuItem(tr("By File Owner"), FolderModel::ColumnFileOwner);

  int col = model->sortColumn();
  if(col >=0 && col < FolderModel::NumOfColumns) {
    sortActions_[col]->setChecked(true);;
  }

  sortMenu_->addSeparator();

  QActionGroup* group = new QActionGroup(this);
  group->setExclusive(true);
  actionAscending_ = new QAction(tr("Ascending"), this);
  actionAscending_->setCheckable(true);
  sortMenu_->addAction(actionAscending_);
  group->addAction(actionAscending_);
  
  actionDescending_ = new QAction(tr("Descending"), this);
  actionDescending_->setCheckable(true);
  sortMenu_->addAction(actionDescending_);
  group->addAction(actionDescending_);

  if(model->sortOrder() == Qt::AscendingOrder)
    actionAscending_->setChecked(true);
  else
    actionDescending_->setChecked(true);

  connect(actionAscending_, SIGNAL(triggered(bool)), SLOT(onSortOrderActionTriggered(bool)));
  connect(actionDescending_, SIGNAL(triggered(bool)), SLOT(onSortOrderActionTriggered(bool)));

  sortMenu_->addSeparator();
  
  QAction* actionFolderFirst = new QAction(tr("Folder First"), this);
  sortMenu_->addAction(actionFolderFirst);
  actionFolderFirst->setCheckable(true);
  if(model->folderFirst())
    actionFolderFirst->setChecked(true);
  connect(actionFolderFirst, SIGNAL(triggered(bool)), SLOT(onFolderFirstActionTriggered(bool)));

  QAction* actionCaseSensitive = new QAction(tr("Case Sensitive"), this);
  sortMenu_->addAction(actionCaseSensitive);
  actionCaseSensitive->setCheckable(true);
  if(model->sortCaseSensitivity() == Qt::CaseSensitive)
    actionCaseSensitive->setChecked(true);
  connect(actionCaseSensitive, SIGNAL(triggered(bool)), SLOT(onCaseSensitiveActionTriggered(bool)));
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

void FolderMenu::onSortActionTriggered(bool checked) {
  ProxyFolderModel* model = view_->model();
  if(model) {
    QAction* action = static_cast<QAction*>(sender());
    for(int col = 0; col < FolderModel::NumOfColumns; ++col) {
      if(action == sortActions_[col]) {
        model->sort(col, model->sortOrder());
        break;
      }
    }
  }
}

void FolderMenu::onSortOrderActionTriggered(bool checked) {
  ProxyFolderModel* model = view_->model();
  if(model) {
    QAction* action = static_cast<QAction*>(sender());
    Qt::SortOrder order;
    if(action == actionAscending_)
      order = Qt::AscendingOrder;
    else
      order = Qt::DescendingOrder;
    model->sort(model->sortColumn(), order);
  }
}

void FolderMenu::onShowHiddenActionTriggered(bool checked) {
  ProxyFolderModel* model = view_->model();
  if(model) {
    qDebug("show hidden: %d", checked);
    model->setShowHidden(checked);
  }
}

void FolderMenu::onCaseSensitiveActionTriggered(bool checked) {
  ProxyFolderModel* model = view_->model();
  if(model) {
    model->setSortCaseSensitivity(checked ? Qt::CaseSensitive : Qt::CaseInsensitive);
  }
}

void FolderMenu::onFolderFirstActionTriggered(bool checked) {
  ProxyFolderModel* model = view_->model();
  if(model) {
    model->setFolderFirst(checked);
  }
}

void FolderMenu::onPropertiesActionTriggered() {
  FmFileInfo* folderInfo = view_->folderInfo();
  if(folderInfo)
    FilePropsDialog::showForFile(folderInfo);
}

#include "foldermenu.moc"
