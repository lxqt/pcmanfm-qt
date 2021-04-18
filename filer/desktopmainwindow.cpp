/*

    Copyright (C) 2021 Chris Moore <chris@mooreonline.org>

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

#include <QDebug>

#include "desktopmainwindow.h"
#include "bookmarkaction.h"
#include "application.h"
#include "gotofolderwindow.h"

using namespace Filer;

DesktopMainWindow::DesktopMainWindow(QWidget *parent) : QMainWindow(parent)
{
  ui.setupUi(this);

  // Delete the actions that are not applicable to the desktop - they will
  // be removed from the menubar
  delete ui.actionNewWin;
  delete ui.actionCloseTab;
  delete ui.actionCloseWindow;
  delete ui.actionIconView;
  delete ui.actionCompactView;
  delete ui.actionDetailedList;
  delete ui.actionThumbnailView;
  delete ui.actionFilter;
  delete ui.actionGoBack;
  delete ui.actionGoForward;
  delete ui.actionGo;
  delete ui.actionOpenAsRoot;

  // load bookmark menu
  bookmarks = fm_bookmarks_dup();
  g_signal_connect(bookmarks, "changed", G_CALLBACK(onBookmarksChanged), this);
  loadBookmarksMenu();
}

QMenuBar *DesktopMainWindow::getMenuBar() const
{
  return ui.menubar;
}

void DesktopMainWindow::setShowHidden(bool hidden)
{
  ui.actionShowHidden->setChecked(hidden);
}

void DesktopMainWindow::setSortColumn(int column)
{
  ui.actionByFileName->setChecked(column == 0);
  ui.actionByFileType->setChecked(column == 1);
  ui.actionByFileSize->setChecked(column == 2);
  ui.actionByMTime->setChecked(column == 3);
  ui.actionByOwner->setChecked(column == 4);
}

void DesktopMainWindow::setSortOrder(Qt::SortOrder order)
{
  ui.actionAscending->setChecked(order == Qt::AscendingOrder);
  ui.actionDescending->setChecked(order == Qt::DescendingOrder);
}

void DesktopMainWindow::setFolderFirst(bool first)
{
  ui.actionFolderFirst->setChecked(first);
}

void DesktopMainWindow::setCaseSensitive(Qt::CaseSensitivity sensitivity)
{
  ui.actionCaseSensitive->setChecked(sensitivity == Qt::CaseSensitive);
}

void DesktopMainWindow::on_actionNewFolder_triggered()
{
  Q_EMIT newFolder();
}

void DesktopMainWindow::on_actionNewBlankFile_triggered()
{
  Q_EMIT newBlankFile();
}

void DesktopMainWindow::on_actionFileProperties_triggered()
{
  Q_EMIT fileProperties();
}

void DesktopMainWindow::on_actionCut_triggered()
{
  Q_EMIT cut();
}

void DesktopMainWindow::on_actionCopy_triggered()
{
  Q_EMIT copy();
}

void DesktopMainWindow::on_actionPaste_triggered()
{
  Q_EMIT paste();
}

void DesktopMainWindow::on_actionDelete_triggered()
{
  Q_EMIT del();
}

void DesktopMainWindow::on_actionRename_triggered()
{
  Q_EMIT rename();
}

void DesktopMainWindow::on_actionSelectAll_triggered()
{
  Q_EMIT selectAll();
}

void DesktopMainWindow::on_actionInvertSelection_triggered()
{
  Q_EMIT invert();
}

void DesktopMainWindow::on_actionPreferences_triggered()
{
  Q_EMIT preferences();
}

void DesktopMainWindow::on_actionGoUp_triggered()
{
  Q_EMIT goUp();
}

void DesktopMainWindow::on_actionHome_triggered()
{
  Q_EMIT openHome();
}

void DesktopMainWindow::on_actionReload_triggered()
{
  Q_EMIT reload();
}

void DesktopMainWindow::on_actionGoToFolder_triggered()
{
  GotoFolderDialog* gotoFolderDialog = new GotoFolderDialog(this);
  int code = gotoFolderDialog->exec();
  if (code == QDialog::Accepted) {
    Q_EMIT openFolder(gotoFolderDialog->getPath());
  }
}

void DesktopMainWindow::on_actionShowHidden_triggered(bool check)
{
  Q_EMIT showHidden(check);
}

void DesktopMainWindow::on_actionByFileName_triggered(bool checked)
{
  Q_EMIT sortColumn(0);
}

void DesktopMainWindow::on_actionByMTime_triggered(bool checked)
{
  Q_EMIT sortColumn(3);
}

void DesktopMainWindow::on_actionByOwner_triggered(bool checked)
{
  Q_EMIT sortColumn(4);
}

void DesktopMainWindow::on_actionByFileType_triggered(bool checked)
{
  Q_EMIT sortColumn(1);
}

void DesktopMainWindow::on_actionByFileSize_triggered(bool checked)
{
  Q_EMIT sortColumn(2);
}

void DesktopMainWindow::on_actionAscending_triggered(bool checked)
{
  Q_EMIT sortOrder(Qt::AscendingOrder);
}

void DesktopMainWindow::on_actionDescending_triggered(bool checked)
{
  Q_EMIT sortOrder(Qt::DescendingOrder);
}

void DesktopMainWindow::on_actionFolderFirst_triggered(bool checked)
{
  Q_EMIT folderFirst(checked);
}

void DesktopMainWindow::on_actionCaseSensitive_triggered(bool checked)
{
  Q_EMIT caseSensitive(checked ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

void DesktopMainWindow::on_actionApplications_triggered()
{
  Q_EMIT openFolder(QString("file:///Applications"));
}

void DesktopMainWindow::on_actionUtilities_triggered()
{
  Q_EMIT openFolder(QString("file:///Applications/Utilities"));
}

void DesktopMainWindow::on_actionDocuments_triggered()
{
  Q_EMIT openDocuments();
}

void DesktopMainWindow::on_actionDownloads_triggered()
{
  Q_EMIT openDownloads();
}

void DesktopMainWindow::on_actionComputer_triggered()
{
  Q_EMIT openFolder(QString("computer:///"));
}

void DesktopMainWindow::on_actionTrash_triggered()
{
  Q_EMIT openTrash();
}

void DesktopMainWindow::on_actionNetwork_triggered()
{
  Q_EMIT openFolder(QString("network:///"));
}

void DesktopMainWindow::on_actionDesktop_triggered()
{
  Q_EMIT openDesktop();
}

void DesktopMainWindow::on_actionEditBookmarks_triggered()
{
  Q_EMIT editBookmarks();
}

void DesktopMainWindow::on_actionOpenTerminal_triggered()
{
  Q_EMIT openTerminal();
}

void DesktopMainWindow::on_actionFindFiles_triggered()
{
  Q_EMIT search();
}

void DesktopMainWindow::on_actionAbout_triggered()
{
  Q_EMIT about();
}

void DesktopMainWindow::onBookmarkActionTriggered()
{
  Fm::BookmarkAction* action = static_cast<Fm::BookmarkAction*>(sender());
  FmPath* path = action->path();
  if(path) {
    qDebug() << "bookmark path: " << fm_path_to_str(path);
    Q_EMIT openFolder(fm_path_to_str(path));
  }
}

void DesktopMainWindow::onBookmarksChanged(FmBookmarks* bookmarks, DesktopMainWindow* pThis) {
  // delete existing items
  QList<QAction*> actions = pThis->ui.menu_Bookmarks->actions();
  QList<QAction*>::const_iterator it = actions.begin();
  QList<QAction*>::const_iterator last_it = actions.end() - 2;

  while(it != last_it) {
    QAction* action = *it;
    ++it;
    pThis->ui.menu_Bookmarks->removeAction(action);
  }

  pThis->loadBookmarksMenu();
}

void DesktopMainWindow::loadBookmarksMenu() {
  GList* allBookmarks = fm_bookmarks_get_all(bookmarks);
  QAction* before = ui.actionAddToBookmarks;

  for(GList* l = allBookmarks; l; l = l->next) {
    FmBookmarkItem* item = reinterpret_cast<FmBookmarkItem*>(l->data);
    Fm::BookmarkAction* action = new Fm::BookmarkAction(item, ui.menu_Bookmarks);
    connect(action, &QAction::triggered, this, &DesktopMainWindow::onBookmarkActionTriggered);
    ui.menu_Bookmarks->insertAction(before, action);
  }

  ui.menu_Bookmarks->insertSeparator(before);
  g_list_free_full(allBookmarks, (GDestroyNotify)fm_bookmark_item_unref);
}
