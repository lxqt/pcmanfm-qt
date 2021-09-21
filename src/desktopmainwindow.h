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

#ifndef DESKTOPMAINWINDOW_H
#define DESKTOPMAINWINDOW_H

#include "ui_main-win.h"
#include <QMainWindow>

namespace Filer {

/*
 * We need to re-use the menubar from the UI of MainWindow for the desktop, so
 * this class sets up the same UI, removes actions from the menubar that are not
 * applicable to the desktop and then emits signals for the ones that are.
 * DesktopWindow then takes the actions for those signals.
 */
class DesktopMainWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit DesktopMainWindow(QWidget *parent = nullptr);

  QMenuBar* getMenuBar() const;

  void setShowHidden(bool hidden);
  void setSortColumn(int column);
  void setSortOrder(Qt::SortOrder order);
  void setFolderFirst(bool first);
  void setCaseSensitive(Qt::CaseSensitivity sensitivity);

protected Q_SLOTS:
  void on_actionOpen_triggered();
  void on_actionShowContents_triggered();

  void on_actionNewFolder_triggered();
  void on_actionNewBlankFile_triggered();
  void on_actionFileProperties_triggered();

  void on_actionCut_triggered();
  void on_actionCopy_triggered();
  void on_actionPaste_triggered();
  void on_actionDelete_triggered();
  void on_actionDuplicate_triggered();
  void on_actionEmptyTrash_triggered();
  void on_actionRename_triggered();
  void on_actionSelectAll_triggered();
  void on_actionInvertSelection_triggered();
  void on_actionPreferences_triggered();

  void on_actionGoUp_triggered();
  void on_actionHome_triggered();
  void on_actionReload_triggered();

  void on_actionGoToFolder_triggered();

  void on_actionShowHidden_triggered(bool check);

  void on_actionByFileName_triggered(bool checked);
  void on_actionByMTime_triggered(bool checked);
  void on_actionByOwner_triggered(bool checked);
  void on_actionByFileType_triggered(bool checked);
  void on_actionByFileSize_triggered(bool checked);
  void on_actionAscending_triggered(bool checked);
  void on_actionDescending_triggered(bool checked);
  void on_actionFolderFirst_triggered(bool checked);
  void on_actionCaseSensitive_triggered(bool checked);

  void on_actionApplications_triggered();
  void on_actionUtilities_triggered();
  void on_actionDocuments_triggered();
  void on_actionDownloads_triggered();
  void on_actionComputer_triggered();
  void on_actionTrash_triggered();
  void on_actionNetwork_triggered();
  void on_actionDesktop_triggered();
  void on_actionEditBookmarks_triggered();

  void on_actionOpenTerminal_triggered();
  void on_actionFindFiles_triggered();

  void on_actionAbout_triggered();

  void onBookmarkActionTriggered();

Q_SIGNALS:
  void open();
  void showContents(); // probono
  void newFolder();
  void newBlankFile();
  void fileProperties();
  void preferences();
  void openFolder(QString folder);
  void openTrash();
  void openDesktop();
  void openDocuments();
  void openDownloads();
  void openHome();
  void goUp();
  void cut();
  void copy();
  void paste();
  void duplicate();
  void emptyTrash();
  void rename();
  void del();
  void selectAll();
  void invert();
  void openTerminal();
  void search();
  void about();
  void editBookmarks();
  void showHidden(bool hidden);
  void sortColumn(int column);
  void sortOrder(Qt::SortOrder order);
  void folderFirst(bool first);
  void caseSensitive(Qt::CaseSensitivity sensitive);
  void reload();

private:
  static void onBookmarksChanged(FmBookmarks* bookmarks, DesktopMainWindow* pThis);
  void loadBookmarksMenu();

private:
    Ui::MainWindow ui;
    FmBookmarks* bookmarks;
};


}

#endif // DESKTOPMAINWINDOW_H
