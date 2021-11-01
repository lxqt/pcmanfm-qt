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

#include "mainwindow.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QSplitter>
#include <QToolButton>
#include <QShortcut>
#include <QKeySequence>
#include <QDebug>
#include <QCompleter>
#include <QFileSystemModel>
#include <QStandardPaths>

#include "tabpage.h"
#include "filelauncher.h"
#include "filemenu.h"
#include "bookmarkaction.h"
#include "fileoperation.h"
#include "utilities.h"
#include "filepropsdialog.h"
#include "pathedit.h"
#include "ui_about.h"
#include "application.h"
#include "path.h"
#include "metadata.h"
#include "windowregistry.h"
#include "gotofolderwindow.h"
#include "trash.h"

// #include "qmodeltest/modeltest.h"

#include <X11/Xlib.h>

using namespace Fm;

namespace Filer {

MainWindow::MainWindow(FmPath* path):
  QMainWindow(),
  fileLauncher_(NULL){

  Settings& settings = static_cast<Application*>(qApp)->settings();
  setAttribute(Qt::WA_DeleteOnClose);
  // setup user interface
  ui.setupUi(this);

  // hide menu items that are not usable
  //if(!uriExists("computer:///"))
  //  ui.actionComputer->setVisible(false);
  if(!settings.supportTrash())
    ui.actionTrash->setVisible(false);

  // FIXME: add an option to hide network:///
  // We cannot use uriExists() here since calling this on "network:///"
  // is very slow and blocking.
  //if(!uriExists("network:///"))
  //  ui.actionNetwork->setVisible(false);

  // add a context menu for showing browse history to back and forward buttons
  QToolButton* forwardButton = static_cast<QToolButton*>(ui.toolBar->widgetForAction(ui.actionGoForward));
  forwardButton->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(forwardButton, &QToolButton::customContextMenuRequested, this, &MainWindow::onBackForwardContextMenu);
  QToolButton* backButton = static_cast<QToolButton*>(ui.toolBar->widgetForAction(ui.actionGoBack));
  backButton->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(backButton, &QToolButton::customContextMenuRequested, this, &MainWindow::onBackForwardContextMenu);

  // tabbed browsing interface
  ui.tabBar->setDocumentMode(true);
  ui.tabBar->setTabsClosable(true);
  ui.tabBar->setElideMode(Qt::ElideRight);
  ui.tabBar->setExpanding(false);
  ui.tabBar->setMovable(true); // reorder the tabs by dragging

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
  // switch to the tab under the cursor during dnd.
  ui.tabBar->setChangeCurrentOnDrag(true);
  ui.tabBar->setAcceptDrops(true);
#endif

  connect(ui.tabBar, &QTabBar::currentChanged, this, &MainWindow::onTabBarCurrentChanged);
  connect(ui.tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::onTabBarCloseRequested);
  connect(ui.tabBar, &QTabBar::tabMoved, this, &MainWindow::onTabBarTabMoved);
  connect(ui.stackedWidget, &QStackedWidget::widgetRemoved, this, &MainWindow::onStackedWidgetWidgetRemoved);

  // FIXME: should we make the filter bar a per-view configuration?
  ui.filterBar->setVisible(settings.showFilter());
  ui.actionFilter->setChecked(settings.showFilter());
  connect(ui.filterBar, &QLineEdit::textChanged, this, &MainWindow::onFilterStringChanged);

  // side pane
  ui.sidePane->setIconSize(QSize(settings.sidePaneIconSize(), settings.sidePaneIconSize()));
  ui.sidePane->setMode(settings.sidePaneMode());
  connect(ui.sidePane, &Fm::SidePane::chdirRequested, this, &MainWindow::onSidePaneChdirRequested);
  connect(ui.sidePane, &Fm::SidePane::openFolderInNewWindowRequested, this, &MainWindow::onSidePaneOpenFolderInNewWindowRequested);
  connect(ui.sidePane, &Fm::SidePane::openFolderInNewTabRequested, this, &MainWindow::onSidePaneOpenFolderInNewTabRequested);
  connect(ui.sidePane, &Fm::SidePane::openFolderInTerminalRequested, this, &MainWindow::onSidePaneOpenFolderInTerminalRequested);
  connect(ui.sidePane, &Fm::SidePane::createNewFolderRequested, this, &MainWindow::onSidePaneCreateNewFolderRequested);
  connect(ui.sidePane, &Fm::SidePane::modeChanged, this, &MainWindow::onSidePaneModeChanged);

  ui.splitter->setHandleWidth(0); // probono: No handles between side bar and main window content

  // detect change of splitter position
  connect(ui.splitter, &QSplitter::splitterMoved, this, &MainWindow::onSplitterMoved);

  // path bar
  pathEntry = new Fm::PathEdit(this);
  connect(pathEntry, &Fm::PathEdit::returnPressed, this, &MainWindow::onPathEntryReturnPressed);
  ui.toolBar->insertWidget(ui.actionGo, pathEntry);

  // add filesystem info to status bar
  fsInfoLabel = new QLabel(ui.statusbar);
  ui.statusbar->addPermanentWidget(fsInfoLabel);

  // setup the splitter
  ui.splitter->setStretchFactor(1, 1); // only the right pane can be stretched
  QList<int> sizes;
  sizes.append(settings.splitterPos());
  sizes.append(300);
  ui.splitter->setSizes(sizes);

  // load bookmark menu
  bookmarks = fm_bookmarks_dup();
  g_signal_connect(bookmarks, "changed", G_CALLBACK(onBookmarksChanged), this);
  loadBookmarksMenu();

  // Fix the menu groups which is not done by Qt designer
  // To my suprise, this was supported in Qt designer 3 :-(
  QActionGroup* group = new QActionGroup(ui.menu_View);
  group->setExclusive(true);
  group->addAction(ui.actionIconView);
  group->addAction(ui.actionCompactView);
  group->addAction(ui.actionThumbnailView);
  group->addAction(ui.actionDetailedList);

  group = new QActionGroup(ui.menuSorting);
  group->setExclusive(true);
  group->addAction(ui.actionByFileName);
  group->addAction(ui.actionByMTime);
  group->addAction(ui.actionByFileSize);
  group->addAction(ui.actionByFileType);
  group->addAction(ui.actionByOwner);

  group = new QActionGroup(ui.menuSorting);
  group->setExclusive(true);
  group->addAction(ui.actionAscending);
  group->addAction(ui.actionDescending);

  // create shortcuts
  QShortcut* shortcut;
  shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), this);
  connect(shortcut, &QShortcut::activated, pathEntry, static_cast<void (QWidget::*)()>(&Fm::PathEdit::setFocus));

  shortcut = new QShortcut(Qt::ALT + Qt::Key_D, this);
  connect(shortcut, &QShortcut::activated, pathEntry, static_cast<void (QWidget::*)()>(&QWidget::setFocus));

  shortcut = new QShortcut(Qt::CTRL + Qt::Key_Tab, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutNextTab);

  shortcut = new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutPrevTab);

  int i;
  for(i = 0; i < 10; ++i) {
    shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_0 + i), this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutJumpToTab);

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_0 + i), this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutJumpToTab);
  }

  shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), this); // pronono: open
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionOpen_triggered); // probono

  shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this); // probono: put in trash
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionDelete_triggered);

  shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), this); // probono: force delete
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionDeleteWithoutTrash_triggered);

  shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Backspace), this); // probono: force delete
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionDeleteWithoutTrash_triggered);

  if(QToolButton* clearButton = ui.filterBar->findChild<QToolButton*>()) {
    clearButton->setToolTip(tr("Clear text (Ctrl+K)"));
    shortcut = new QShortcut(Qt::CTRL + Qt::Key_K, this);
    connect(shortcut, &QShortcut::activated, ui.filterBar, &QLineEdit::clear);
  }

  if(path)
    addTab(path);

  // size from spatial mode or from settings
  if (settings.spatialMode()) {

    // hide the things we don't want in spatial mode
    ui.tabBar->hide();
    ui.sidePane->hide();
    ui.toolBar->hide();
    ui.frame->layout()->setContentsMargins(0, 0, 0, 0);
    delete ui.actionNewWin; // Will be removed from menubar when window is opened next time; FIXME: Do immediately when Spatial mode mode is set

    // Set the window position and size
    MetaData metaData(fm_path_to_str(path));
    int x, y, width, height;
    bool ok;
    x = metaData.getWindowOriginX(ok);
    if (ok)
      y = metaData.getWindowOriginY(ok);
    if (ok)
      width = metaData.getWindowWidth(ok);
    if (ok)
      height = metaData.getWindowHeight(ok);
    if (ok)
      setGeometry(x, y, width, height);

    // Set the window view mode
    MetaData::FolderView view = metaData.getWindowView(ok);
    if (ok) {
      switch (view) {
      case MetaData::Icons:
        ui.actionIconView->trigger();
        break;
      case MetaData::Compact:
        ui.actionCompactView->trigger();
        break;
      case MetaData::List:
        ui.actionDetailedList->trigger();
        break;
      case MetaData::Thumbnail:
        ui.actionThumbnailView->trigger();
        break;
      case MetaData::Columns:
        // not implemented
        ui.actionDetailedList->trigger();
        break;
      }
    }

    // Set the window sort item
    MetaData::SortItem sortItem = metaData.getWindowSortItem(ok);
    if (ok) {
      switch (sortItem) {
      case MetaData::FileName:
        ui.actionByFileName->trigger();
        break;
      case MetaData::FileType:
        ui.actionByFileType->trigger();
        break;
      case MetaData::FileSize:
        ui.actionByFileSize->trigger();
        break;
      case MetaData::ModifiedTime:
        ui.actionByMTime->trigger();
        break;
      case MetaData::Owner:
        ui.actionByOwner->trigger();
        break;
      }
    }

    // Set the window sort order
    MetaData::SortOrder sortOrder = metaData.getWindowSortOrder(ok);
    if (ok) {
      switch (sortOrder) {
      case MetaData::Ascending:
        ui.actionAscending->trigger();
        break;
      case MetaData::Descending:
        ui.actionDescending->trigger();
        break;
      }
    }

    // Set the window sort case sensitivity
    MetaData::SortCase sortCase = metaData.getWindowSortCase(ok);
    if (ok) {
      switch (sortCase) {
      case MetaData::CaseSensitive:
        ui.actionCaseSensitive->setChecked(true);
        break;
      case MetaData::NotCaseSensitive:
        ui.actionCaseSensitive->setChecked(false);
        break;
      }
    }

    // Set the window 'folder first' sort option
    MetaData::SortFolderFirst folderFirst = metaData.getWindowSortFolderFirst(ok);
    if (ok) {
      switch (folderFirst) {
      case MetaData::FoldersFirst:
        ui.actionFolderFirst->setChecked(true);
        break;
      case MetaData::NotFoldersFirst:
        ui.actionFolderFirst->setChecked(false);
        break;
      }
    }

  }
  else if(settings.rememberWindowSize()) {
    resize(settings.windowWidth(), settings.windowHeight());
    if(settings.windowMaximized())
      setWindowState(windowState() | Qt::WindowMaximized);
  }

  // register current path with the window registry
  WindowRegistry::instance().registerPath(fm_path_to_str(path));
  connect(&WindowRegistry::instance(), &WindowRegistry::raiseWindow,
          this, &MainWindow::onRaiseWindow);
  connect(&WindowRegistry::instance(), &WindowRegistry::raiseWindowAndSelectItems,
          this, &MainWindow::onRaiseWindowAndSelectItems);
}

MainWindow::~MainWindow() {
  // update registry
  TabPage* page = currentPage();
  if(page) {
    QString path = page->pathName();
    WindowRegistry::instance().deregisterPath(path);
  }
  if(bookmarks)
    g_object_unref(bookmarks);
}

void MainWindow::chdir(FmPath* path) {
  if (isSpatialMode()) {
    if ( ! WindowRegistry::instance().checkPathAndRaise(fm_path_to_str(path))) {
        (new MainWindow(path))->show();
    }
  }
  else {
    TabPage* page = currentPage();

    if(page) {
      ui.filterBar->clear();
      page->chdir(path, true);
      updateUIForCurrentPage();
    }
  }
}

// add a new tab
void MainWindow::addTab(FmPath* path) {
  Settings& settings = static_cast<Application*>(qApp)->settings();

  TabPage* newPage = new TabPage(path, this);
  newPage->setFileLauncher(&fileLauncher_);
  int index = ui.stackedWidget->addWidget(newPage);
  connect(newPage, &TabPage::titleChanged, this, &MainWindow::onTabPageTitleChanged);
  connect(newPage, &TabPage::statusChanged, this, &MainWindow::onTabPageStatusChanged);
  connect(newPage, &TabPage::openDirRequested, this, &MainWindow::onTabPageOpenDirRequested);
  connect(newPage, &TabPage::sortFilterChanged, this, &MainWindow::onTabPageSortFilterChanged);
  connect(newPage, &TabPage::backwardRequested, this, &MainWindow::on_actionGoBack_triggered);
  connect(newPage, &TabPage::forwardRequested, this, &MainWindow::on_actionGoForward_triggered);

  ui.tabBar->insertTab(index, newPage->title());

  if(!settings.alwaysShowTabs()) {
    ui.tabBar->setVisible(ui.tabBar->count() > 1);
  }

  // update registry
  WindowRegistry::instance().registerPath(fm_path_to_str(path));
}

void MainWindow::addWindow(FmPath *path)
{
  (new MainWindow(path))->show();
}

void MainWindow::onPathEntryReturnPressed() {
  QString text = pathEntry->text();
  QByteArray utext = text.toUtf8();
  FmPath* path = fm_path_new_for_display_name(utext);
  chdir(path);
  fm_path_unref(path);
}

void MainWindow::on_actionGoUp_triggered() {
  TabPage* page = currentPage();

  if(page) {
    ui.filterBar->clear();
    FmPath* parent = fm_path_get_parent(page->path());
    if (parent)
      chdir(parent);
    updateUIForCurrentPage();
  }
}

void MainWindow::on_actionGoBack_triggered() {
  TabPage* page = currentPage();

  if(page) {
    ui.filterBar->clear();
    page->backward();
    updateUIForCurrentPage();
  }
}

void MainWindow::on_actionGoForward_triggered() {
  TabPage* page = currentPage();

  if(page) {
    ui.filterBar->clear();
    page->forward();
    updateUIForCurrentPage();
  }
}

void MainWindow::on_actionHome_triggered() {
  chdir(fm_path_get_home());
}

void MainWindow::on_actionReload_triggered() {
  currentPage()->reload();
}

void MainWindow::on_actionGo_triggered() {
  onPathEntryReturnPressed();
}

void MainWindow::on_actionNewTab_triggered() {
  FmPath* path = currentPage()->path();
  addTab(path);
}

void MainWindow::on_actionNewWin_triggered() {
  FmPath* path = currentPage()->path();
  (new MainWindow(path))->show();
}

void MainWindow::on_actionNewFolder_triggered() {
  if(TabPage* tabPage = currentPage()) {
    FmPath* dirPath = tabPage->folderView()->path();

    if(dirPath)
      createFileOrFolder(CreateNewFolder, dirPath);
  }
}

void MainWindow::on_actionNewBlankFile_triggered() {
  if(TabPage* tabPage = currentPage()) {
    FmPath* dirPath = tabPage->folderView()->path();

    if(dirPath)
      createFileOrFolder(CreateNewTextFile, dirPath);
  }
}

void MainWindow::on_actionCloseTab_triggered() {
  closeTab(ui.tabBar->currentIndex());
}

void MainWindow::on_actionCloseWindow_triggered() {
  // FIXME: should we save state here?
  close();
  // the window will be deleted automatically on close
}

// probono
void MainWindow::on_actionOpen_triggered() {
    TabPage* page = currentPage();

    if(page) {
        FmFileInfoList* files = page->selectedFiles();

        if(files) {
            if(page->fileLauncher()) {
                page->fileLauncher()->launchFiles(NULL, files);
            }
            else { // use the default launcher
                Fm::FileLauncher launcher;
                launcher.launchFiles(NULL, files);
            }
        }
    }
}

// probono
void MainWindow::on_actionShowContents_triggered() {
    TabPage* page = currentPage();

    if(page) {
        FmFileInfoList* files = page->selectedFiles();

        if(files) {
            if(page->fileLauncher()) {
                page->fileLauncher()->launchFiles(NULL, files, true);
            }
            else { // use the default launcher
                Fm::FileLauncher launcher;
                launcher.launchFiles(NULL, files, true);
            }
        }
    }
}

void MainWindow::on_actionFileProperties_triggered() {
  TabPage* page = currentPage();

  if(page) {
    FmFileInfoList* files = page->selectedFiles();

    if(files) {
      Fm::FilePropsDialog::showForFiles(files);
      fm_file_info_list_unref(files);
    }
  }
}

void MainWindow::on_actionFolderProperties_triggered() {
  TabPage* page = currentPage();

  if(page) {
    FmFolder* folder = page->folder();

    if(folder) {
      FmFileInfo* info = fm_folder_get_info(folder);

      if(info)
        Fm::FilePropsDialog::showForFile(info);
    }
  }
}

void MainWindow::on_actionShowHidden_triggered(bool checked) {
  TabPage* tabPage = currentPage();
  tabPage->setShowHidden(checked);
  ui.sidePane->setShowHidden(checked);
  static_cast<Application*>(qApp)->settings().setShowHidden(checked);
}

void MainWindow::on_actionByFileName_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileName, currentPage()->sortOrder());
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowSortItem(MetaData::SortItem::FileName);
  }
}

void MainWindow::on_actionByMTime_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileMTime, currentPage()->sortOrder());
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowSortItem(MetaData::SortItem::ModifiedTime);
  }
}

void MainWindow::on_actionByOwner_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileOwner, currentPage()->sortOrder());
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowSortItem(MetaData::SortItem::Owner);
  }
}

void MainWindow::on_actionByFileSize_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileSize, currentPage()->sortOrder());
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowSortItem(MetaData::SortItem::FileSize);
  }
}

void MainWindow::on_actionByFileType_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileType, currentPage()->sortOrder());
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowSortItem(MetaData::SortItem::FileType);
  }
}

void MainWindow::on_actionAscending_triggered(bool checked) {
  currentPage()->sort(currentPage()->sortColumn(), Qt::AscendingOrder);
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowSortOrder(MetaData::SortOrder::Ascending);
  }
}

void MainWindow::on_actionDescending_triggered(bool checked) {
  currentPage()->sort(currentPage()->sortColumn(), Qt::DescendingOrder);
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowSortOrder(MetaData::SortOrder::Descending);
  }
}

void MainWindow::on_actionCaseSensitive_triggered(bool checked) {
  currentPage()->setSortCaseSensitive(checked);
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowSortCase(checked ?
                                 MetaData::SortCase::CaseSensitive
                               : MetaData::SortCase::NotCaseSensitive);
  }
}

void MainWindow::on_actionFolderFirst_triggered(bool checked) {
  currentPage()->setSortFolderFirst(checked);
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowSortFolderFirst(checked ?
                                 MetaData::SortFolderFirst::FoldersFirst
                               : MetaData::SortFolderFirst::NotFoldersFirst);
  }
}

void MainWindow::on_actionFilter_triggered(bool checked) {
  ui.filterBar->setVisible(checked);
  static_cast<Application*>(qApp)->settings().setShowFilter(checked);
}

void MainWindow::on_actionComputer_triggered() {
  FmPath* path = fm_path_new_for_uri("computer:///");
  chdir(path);
  fm_path_unref(path);
}

void MainWindow::on_actionApplications_triggered() {
  // chdir(fm_path_get_apps_menu());
  // probono: Use hardcoded /Applications for now
  FmPath* path = fm_path_new_for_uri("file:///Applications");
  chdir(path);
  fm_path_unref(path);
}

void MainWindow::on_actionUtilities_triggered() {
  FmPath* path = fm_path_new_for_uri("file:///Applications/Utilities");
  chdir(path);
  fm_path_unref(path);
}

void MainWindow::on_actionDocuments_triggered() {
  // chdir(fm_path_get_apps_menu());
  FmPath* path;
  path = fm_path_new_for_str(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toLocal8Bit().data());
  chdir(path);
  fm_path_unref(path);
}

void MainWindow::on_actionDownloads_triggered() {
  // chdir(fm_path_get_apps_menu());
  FmPath* path;
  path = fm_path_new_for_str(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation).toLocal8Bit().data());
  chdir(path);
  fm_path_unref(path);
}

void MainWindow::on_actionTrash_triggered() {
  chdir(fm_path_get_trash());
}

void MainWindow::on_actionNetwork_triggered() {
  FmPath* path = fm_path_new_for_uri("network:///");
  chdir(path);
  fm_path_unref(path);
}

void MainWindow::on_actionDesktop_triggered() {
  chdir(fm_path_get_desktop());
}

void MainWindow::on_actionAddToBookmarks_triggered() {
  TabPage* page = currentPage();

  if(page) {
    FmPath* cwd = page->path();

    if(cwd) {
      char* dispName = fm_path_display_basename(cwd);
      fm_bookmarks_insert(bookmarks, cwd, dispName, -1);
      g_free(dispName);
    }
  }
}

void MainWindow::on_actionEditBookmarks_triggered() {
  Application* app = static_cast<Application*>(qApp);
  app->editBookmarks();
}

void MainWindow::on_actionAbout_triggered() {
  // the about dialog
  class AboutDialog : public QDialog {
  public:
    explicit AboutDialog(QWidget* parent = 0, Qt::WindowFlags f = 0) {
      ui.setupUi(this);
      ui.version->setText(tr("Version: %1").arg(FILER_VERSION));
    }
  private:
    Ui::AboutDialog ui;
  };
  AboutDialog dialog(this);
  dialog.exec();
}

void MainWindow::on_actionIconView_triggered() {
  currentPage()->setViewMode(Fm::FolderView::IconMode);
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowView(MetaData::Icons);
  }
}

void MainWindow::on_actionCompactView_triggered() {
  currentPage()->setViewMode(Fm::FolderView::CompactMode);
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowView(MetaData::Compact);
  }
}

void MainWindow::on_actionDetailedList_triggered() {
  currentPage()->setViewMode(Fm::FolderView::DetailedListMode);
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowView(MetaData::List);
  }
}

void MainWindow::on_actionThumbnailView_triggered() {
  currentPage()->setViewMode(Fm::FolderView::ThumbnailMode);
  if (isSpatialMode()) {
    QString path = currentPage()->pathName();
    MetaData metaData(path);
    metaData.setWindowView(MetaData::Thumbnail);
  }
}

void MainWindow::on_actionGoToFolder_triggered() {
  GotoFolderDialog* gotoFolderDialog = new GotoFolderDialog(this);
  int code = gotoFolderDialog->exec();
  if (code == QDialog::Accepted) {
    FmPath* path = fm_path_new_for_path(gotoFolderDialog->getPath().toLatin1().data());
    chdir(path);
    fm_path_unref(path);
  }
}

void MainWindow::onTabBarCloseRequested(int index) {
  closeTab(index);
}

void MainWindow::onTabBarTabMoved(int from, int to) {
  // a tab in the tab bar is moved by the user, so we have to move the
  //  corredponding tab page in the stacked widget to the new position, too.
  QWidget* page = ui.stackedWidget->widget(from);
  if(page) {
	// we're not going to delete the tab page, so here we block signals
	// to avoid calling the slot onStackedWidgetWidgetRemoved() before
	// removing the page. Otherwise the page widget will be destroyed.
    ui.stackedWidget->blockSignals(true);
    ui.stackedWidget->removeWidget(page);
    ui.stackedWidget->insertWidget(to, page); // insert the page to the new position
    ui.stackedWidget->blockSignals(false); // unblock signals
    ui.stackedWidget->setCurrentWidget(page);
  }
}

void MainWindow::onFilterStringChanged(QString str) {
  if(TabPage* tabPage = currentPage()) {
    // appy filter only if needed (not if tab is changed)
    if(str != tabPage->getFilterStr()) {
      tabPage->setFilterStr(str);
      tabPage->applyFilter();
    }
  }
}

void MainWindow::closeTab(int index) {
  // update registry
  if (TabPage* page = static_cast<TabPage*>(ui.stackedWidget->widget(index))) {
    WindowRegistry::instance().deregisterPath(page->pathName());
  }

  QWidget* page = ui.stackedWidget->widget(index);
  if(page) {
    ui.stackedWidget->removeWidget(page); // this does not delete the page widget
    delete page;
    // NOTE: we do not remove the tab here.
    // it'll be donoe in onStackedWidgetWidgetRemoved()
  }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  Settings& settings = static_cast<Application*>(qApp)->settings();
  if (settings.spatialMode()) {
    TabPage* page = currentPage();
    if(page) {
      QString path = page->pathName();
      MetaData metaData(path);
      metaData.setWindowHeight(height());
      metaData.setWindowWidth(width());
    }
  }
  else if(settings.rememberWindowSize()) {
    settings.setLastWindowMaximized(isMaximized());

    if(!isMaximized()) {
        settings.setLastWindowWidth(width());
        settings.setLastWindowHeight(height());
    }
  }
}

void MainWindow::moveEvent(QMoveEvent *event)
{
  QMainWindow::moveEvent(event);
  if (isSpatialMode()) {
    TabPage* page = currentPage();
    if(page) {
      QString path = page->pathName();
      MetaData metaData(path);
      metaData.setWindowOriginX(geometry().x());
      metaData.setWindowOriginY(geometry().y());
    }
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  QWidget::closeEvent(event);
  Settings& settings = static_cast<Application*>(qApp)->settings();
  if(settings.rememberWindowSize()) {
    settings.setLastWindowMaximized(isMaximized());

    if(!isMaximized()) {
        settings.setLastWindowWidth(width());
        settings.setLastWindowHeight(height());
    }
  }
}

void MainWindow::onTabBarCurrentChanged(int index) {
  ui.stackedWidget->setCurrentIndex(index);
  if(TabPage* page = static_cast<TabPage*>(ui.stackedWidget->widget(index)))
    ui.filterBar->setText(page->getFilterStr());
  updateUIForCurrentPage();
}

void MainWindow::updateStatusBarForCurrentPage() {
  TabPage* tabPage = currentPage();
  QString text = tabPage->statusText(TabPage::StatusTextSelectedFiles);
  if(text.isEmpty())
    text = tabPage->statusText(TabPage::StatusTextNormal);
  ui.statusbar->showMessage(text);

  text = tabPage->statusText(TabPage::StatusTextFSInfo);
  fsInfoLabel->setText(text);
  fsInfoLabel->setVisible(!text.isEmpty());
}

bool MainWindow::isSpatialMode() const
{
  Settings& settings = static_cast<Application*>(qApp)->settings();
  return settings.spatialMode();
}

void MainWindow::updateViewMenuForCurrentPage() {
  TabPage* tabPage = currentPage();

  if(tabPage) {
    // update menus. FIXME: should we move this to another method?
    ui.actionShowHidden->setChecked(tabPage->showHidden());

    // view mode
    QAction* modeAction = NULL;

    switch(tabPage->viewMode()) {
    case Fm::FolderView::IconMode:
      modeAction = ui.actionIconView;
      break;

    case Fm::FolderView::CompactMode:
      modeAction = ui.actionCompactView;
      break;

    case Fm::FolderView::DetailedListMode:
      modeAction = ui.actionDetailedList;
      break;

    case Fm::FolderView::ThumbnailMode:
      modeAction = ui.actionThumbnailView;
      break;
    }

    Q_ASSERT(modeAction != NULL);
    modeAction->setChecked(true);

    // sort menu
    QAction* sortActions[Fm::FolderModel::NumOfColumns];
    sortActions[Fm::FolderModel::ColumnFileName] = ui.actionByFileName;
    sortActions[Fm::FolderModel::ColumnFileMTime] = ui.actionByMTime;
    sortActions[Fm::FolderModel::ColumnFileSize] = ui.actionByFileSize;
    sortActions[Fm::FolderModel::ColumnFileType] = ui.actionByFileType;
    sortActions[Fm::FolderModel::ColumnFileOwner] = ui.actionByOwner;
    sortActions[tabPage->sortColumn()]->setChecked(true);

    if(tabPage->sortOrder() == Qt::AscendingOrder)
      ui.actionAscending->setChecked(true);
    else
      ui.actionDescending->setChecked(true);

    ui.actionCaseSensitive->setChecked(tabPage->sortCaseSensitive());
    ui.actionFolderFirst->setChecked(tabPage->sortFolderFirst());
  }
}

void MainWindow::updateUIForCurrentPage() {

  TabPage* tabPage = currentPage();

  if(tabPage) {
    // probono: Whenever we switch to a tab, see whether the user has items selected and enable/disable menu items accordingly
    if(currentPage()->selectedFiles() == 0x0)
      disableMenuItems();
    else
      enableMenuItems();

    setWindowTitle(tabPage->title());
    pathEntry->setText(tabPage->pathName());
    ui.statusbar->showMessage(tabPage->statusText());
    fsInfoLabel->setText(tabPage->statusText(TabPage::StatusTextFSInfo));
    tabPage->folderView()->childView()->setFocus();

    // update side pane
    ui.sidePane->setCurrentPath(tabPage->path());
    ui.sidePane->setShowHidden(tabPage->showHidden());

    // update back/forward/up toolbar buttons
    ui.actionGoUp->setEnabled(tabPage->canUp());
    ui.actionGoBack->setEnabled(tabPage->canBackward());
    ui.actionGoForward->setEnabled(tabPage->canForward());

    updateViewMenuForCurrentPage();
    updateStatusBarForCurrentPage();
  }
}

void MainWindow::onStackedWidgetWidgetRemoved(int index) {
  // qDebug("onStackedWidgetWidgetRemoved: %d", index);
  // need to remove associated tab from tabBar
  ui.tabBar->removeTab(index);
  if(ui.tabBar->count() == 0) { // this is the last one
    deleteLater(); // destroy the whole window
    // qDebug("delete window");
  }
  else {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    if(!settings.alwaysShowTabs() && ui.tabBar->count() == 1) {
      ui.tabBar->setVisible(false);
    }
  }
}

void MainWindow::onTabPageTitleChanged(QString title) {
  TabPage* tabPage = static_cast<TabPage*>(sender());
  int index = ui.stackedWidget->indexOf(tabPage);

  if(index >= 0)
    ui.tabBar->setTabText(index, title);

  if(tabPage == currentPage())
    setWindowTitle(title);
}

void Filer::MainWindow::disableMenuItems()
{
    // probono: No object has been selected by the user, so disable the actions that work on filesystem objects
    // qDebug() << "probono: disableMenuItems";
    ui.actionOpen->setEnabled(false);
    ui.actionFileProperties->setEnabled(false);
    ui.actionCut->setEnabled(false);
    ui.actionCopy->setEnabled(false);
    ui.actionDuplicate->setEnabled(false);
    ui.actionRename->setEnabled(false);
    ui.actionTrash->setEnabled(false);
    ui.actionShowContents->setEnabled(false);
}

void Filer::MainWindow::enableMenuItems()
{
    // probono: At least one object has been selected, so enable the actions that work on filesystem objects
    // qDebug() << "probono: enableMenuItems";
    ui.actionOpen->setEnabled(true);
    ui.actionFileProperties->setEnabled(true);
    ui.actionCut->setEnabled(true);
    ui.actionCopy->setEnabled(true);
    ui.actionDuplicate->setEnabled(true);
    ui.actionRename->setEnabled(true);
    ui.actionTrash->setEnabled(true);
    ui.actionShowContents->setEnabled(true);
}

void MainWindow::onTabPageStatusChanged(int type, QString statusText) {
  TabPage* tabPage = static_cast<TabPage*>(sender());
  if(tabPage == currentPage()) {
    switch(type) {
    case TabPage::StatusTextNormal:
    case TabPage::StatusTextSelectedFiles: {
      // FIXME: updating the status text so frequently is a little bit ineffiecient
      QString text = statusText = tabPage->statusText(TabPage::StatusTextSelectedFiles);
      if(text.isEmpty()) {
        ui.statusbar->showMessage(tabPage->statusText(TabPage::StatusTextNormal));
        disableMenuItems(); // probono
      } else {
        ui.statusbar->showMessage(text);
        enableMenuItems(); // probono
      }
      break;
    }
    case TabPage::StatusTextFSInfo:
      fsInfoLabel->setText(tabPage->statusText(TabPage::StatusTextFSInfo));
      fsInfoLabel->setVisible(!statusText.isEmpty());
      break;
    }
  }
}

void MainWindow::onTabPageOpenDirRequested(FmPath* path, int target) {
  switch(target) {
  case OpenInCurrentTab:
    chdir(path);
    break;

  case OpenInNewTab:
    addTab(path);
    break;

  case OpenInNewWindow: {
    (new MainWindow(path))->show();
    break;
  }
  }
}

void MainWindow::onTabPageSortFilterChanged() {
  TabPage* tabPage = static_cast<TabPage*>(sender());

  if(tabPage == currentPage()) {
    updateViewMenuForCurrentPage();
    Settings& settings = static_cast<Application*>(qApp)->settings();
    settings.setSortColumn(static_cast<Fm::FolderModel::ColumnId>(tabPage->sortColumn()));
    settings.setSortOrder(tabPage->sortOrder());
    settings.setSortFolderFirst(tabPage->sortFolderFirst());

    // Update metadata - this is when the user click on column headings to
    // sort, rather than selecting items to sort in the menu
    if (isSpatialMode()) {

      QString path = currentPage()->pathName();
      MetaData metaData(path);

      switch (static_cast<Fm::FolderModel::ColumnId>(tabPage->sortColumn())) {
      case Fm::FolderModel::ColumnFileName:
        metaData.setWindowSortItem(MetaData::SortItem::FileName);
        break;
      case Fm::FolderModel::ColumnFileType:
        metaData.setWindowSortItem(MetaData::SortItem::FileType);
        break;
      case Fm::FolderModel::ColumnFileSize:
        metaData.setWindowSortItem(MetaData::SortItem::FileSize);
        break;
      case Fm::FolderModel::ColumnFileMTime:
        metaData.setWindowSortItem(MetaData::SortItem::ModifiedTime);
        break;
      case Fm::FolderModel::ColumnFileOwner:
        metaData.setWindowSortItem(MetaData::SortItem::Owner);
        break;
      case Fm::FolderModel::NumOfColumns:
        break;
      }

      switch (tabPage->sortOrder()) {
      case Qt::AscendingOrder:
        metaData.setWindowSortOrder(MetaData::SortOrder::Ascending);
        break;
      case Qt::DescendingOrder:
        metaData.setWindowSortOrder(MetaData::SortOrder::Descending);
        break;
      }

      metaData.setWindowSortFolderFirst(tabPage->sortFolderFirst() ?
                                          MetaData::SortFolderFirst::FoldersFirst
                                        : MetaData::SortFolderFirst::NotFoldersFirst);

    }
  }
}


void MainWindow::onSidePaneChdirRequested(int type, FmPath* path) {
  // FIXME: use enum for type value or change it to button.
  if(type == 0) // left button (default)
    chdir(path);
  else if(type == 1) // middle button
    addTab(path);
  else if(type == 2) // new window
    (new MainWindow(path))->show();
}

void MainWindow::onSidePaneOpenFolderInNewWindowRequested(FmPath* path) {
  (new MainWindow(path))->show();
}

void MainWindow::onSidePaneOpenFolderInNewTabRequested(FmPath* path) {
  addTab(path);
}

void MainWindow::onSidePaneOpenFolderInTerminalRequested(FmPath* path) {
  Application* app = static_cast<Application*>(qApp);
  app->openFolderInTerminal(path);
}

void MainWindow::onSidePaneCreateNewFolderRequested(FmPath* path) {
  createFileOrFolder(CreateNewFolder, path);
}

void MainWindow::onSidePaneModeChanged(Fm::SidePane::Mode mode) {
  static_cast<Application*>(qApp)->settings().setSidePaneMode(mode);
}

void MainWindow::onSplitterMoved(int pos, int index) {
  Application* app = static_cast<Application*>(qApp);
  app->settings().setSplitterPos(pos);
}

void MainWindow::loadBookmarksMenu() {
  GList* allBookmarks = fm_bookmarks_get_all(bookmarks);
  QAction* before = ui.actionAddToBookmarks;

  for(GList* l = allBookmarks; l; l = l->next) {
    FmBookmarkItem* item = reinterpret_cast<FmBookmarkItem*>(l->data);
    BookmarkAction* action = new BookmarkAction(item, ui.menu_Bookmarks);
    connect(action, &QAction::triggered, this, &MainWindow::onBookmarkActionTriggered);
    ui.menu_Bookmarks->insertAction(before, action);
  }

  ui.menu_Bookmarks->insertSeparator(before);
  g_list_free_full(allBookmarks, (GDestroyNotify)fm_bookmark_item_unref);
}

void MainWindow::onBookmarksChanged(FmBookmarks* bookmarks, MainWindow* pThis) {
  // delete existing items
  if ( ! pThis->ui.menu_Bookmarks )
    return;

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

void MainWindow::onBookmarkActionTriggered() {
  BookmarkAction* action = static_cast<BookmarkAction*>(sender());
  FmPath* path = action->path();
  if(path) {
    Application* app = static_cast<Application*>(qApp);
    Settings& settings = app->settings();
    switch(settings.bookmarkOpenMethod()) {
    case OpenInCurrentTab: /* current tab */
    default:
      chdir(path);
      break;
    case OpenInNewTab: /* new tab */
      addTab(path);
      break;
    case OpenInNewWindow: /* new window */
      (new MainWindow(path))->show();
      break;
    }
  }
}

void MainWindow::on_actionCopy_triggered() {
  TabPage* page = currentPage();
  FmPathList* paths = page->selectedFilePaths();
  copyFilesToClipboard(paths);
  fm_path_list_unref(paths);
}

void MainWindow::on_actionCut_triggered() {
  TabPage* page = currentPage();
  FmPathList* paths = page->selectedFilePaths();
  cutFilesToClipboard(paths);
  fm_path_list_unref(paths);
}

void MainWindow::on_actionPaste_triggered() {
  pasteFilesFromClipboard(currentPage()->path(), this);
}

void MainWindow::on_actionDuplicate_triggered() {
  on_actionCopy_triggered();
  on_actionPaste_triggered();
}

void MainWindow::on_actionEmptyTrash_triggered() {
  Fm::Trash::emptyTrash();
}

void MainWindow::on_actionDelete_triggered() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  TabPage* page = currentPage();
  FmPathList* paths = page->selectedFilePaths();
  FileOperation::trashFiles(paths, settings.confirmTrash(), this);
  fm_path_list_unref(paths);
}

void MainWindow::on_actionDeleteWithoutTrash_triggered() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  TabPage* page = currentPage();
  FmPathList* paths = page->selectedFilePaths();
  FileOperation::deleteFiles(paths, settings.confirmDelete(), this);
  fm_path_list_unref(paths);
}

void MainWindow::on_actionRename_triggered() {
  TabPage* page = currentPage();
  FmFileInfoList* files = page->selectedFiles();

  for(GList* l = fm_file_info_list_peek_head_link(files); l; l = l->next) {
    FmFileInfo* file = FM_FILE_INFO(l->data);
    Fm::renameFile(file, NULL);
  }
  fm_file_info_list_unref(files);
}

void MainWindow::on_actionSelectAll_triggered() {
  currentPage()->selectAll();
}

void MainWindow::on_actionInvertSelection_triggered() {
  currentPage()->invertSelection();
}

void MainWindow::on_actionPreferences_triggered() {
  Application* app = reinterpret_cast<Application*>(qApp);
  app->preferences(QString());
}

void MainWindow::onBackForwardContextMenu(QPoint pos) {
  // show a popup menu for browsing history here.
  QToolButton* btn = static_cast<QToolButton*>(sender());
  TabPage* page = currentPage();
  Fm::BrowseHistory& history = page->browseHistory();
  int current = history.currentIndex();
  QMenu menu;
  for(int i = 0; i < history.size(); ++i) {
    const BrowseHistoryItem& item = history.at(i);
    Fm::Path path = item.path();
    QAction* action = menu.addAction(path.displayName());
    if(i == current) {
      // make the current path bold and checked
      action->setCheckable(true);
      action->setChecked(true);
      QFont font = menu.font();
      font.setBold(true);
      action->setFont(font);
    }
  }
  QAction* selectedAction = menu.exec(btn->mapToGlobal(pos));
  if(selectedAction) {
    int index = menu.actions().indexOf(selectedAction);
    ui.filterBar->clear();
    page->jumpToHistory(index);
    updateUIForCurrentPage();
  }
}

void MainWindow::onRaiseWindow(const QString& path)
{
  // all tab pages
  int n = ui.stackedWidget->count();
  for(int i = 0; i < n; ++i) {
    TabPage* page = static_cast<TabPage*>(ui.stackedWidget->widget(i));
    if (page) {
      QString ourPath = page->pathName();
      if (path == ourPath) {
        raise();
        activateWindow();
        // Bring Filer to front
        Display *dpy;
        dpy = XOpenDisplay(NULL);
        XRaiseWindow(dpy, effectiveWinId());
        XCloseDisplay(dpy);
        bool maximized = isMaximized();
        if (isMinimized()) {
          showNormal();
          if (maximized) // window was maximized before being minimized - showNormal restores it
            showMaximized();
        }
        break;
      }
    }
  }
}

void MainWindow::onRaiseWindowAndSelectItems(const QString& path, const QStringList& items) {
    onRaiseWindow(path);
    TabPage* page = currentPage();
    if(page) {
      page->folderView()->selectFiles(items, false);
    }
}

void MainWindow::updateFromSettings(Settings& settings) {
  // apply settings

  // side pane
  ui.sidePane->setIconSize(QSize(settings.sidePaneIconSize(), settings.sidePaneIconSize()));

  // tabs
  ui.tabBar->setTabsClosable(settings.showTabClose());

  if(!settings.alwaysShowTabs()) {
    ui.tabBar->setVisible(ui.tabBar->count() > 1);
  }

  // all tab pages
  int n = ui.stackedWidget->count();

  for(int i = 0; i < n; ++i) {
    TabPage* page = static_cast<TabPage*>(ui.stackedWidget->widget(i));
    page->updateFromSettings(settings);
  }

  // spatial mode
  ui.tabBar->setVisible( ! settings.spatialMode() );
  ui.sidePane->setVisible( ! settings.spatialMode() );
  ui.toolBar->setVisible( ! settings.spatialMode() );
  ui.frame->layout()->setContentsMargins(settings.spatialMode() ? 0 : 1, 0, 0, 0);
}

static const char* su_cmd_subst(char opt, gpointer user_data) {
  return (const char*)user_data;
}

static FmAppCommandParseOption su_cmd_opts[] = {
  { 's', su_cmd_subst },
  { 0, NULL }
};

void MainWindow::on_actionOpenAsRoot_triggered() {
  TabPage* page = currentPage();

  if(page) {
    Application* app = static_cast<Application*>(qApp);
    Settings& settings = app->settings();

    if(!settings.suCommand().isEmpty()) {
      // run the su command
      // FIXME: it's better to get the filename of the current process rather than hard-code filer-qt here.
      QByteArray suCommand = settings.suCommand().toLocal8Bit();
      char* cmd = NULL;
      QByteArray programCommand = app->applicationFilePath().toLocal8Bit();
      programCommand += " %U";

      if(fm_app_command_parse(suCommand.constData(), su_cmd_opts, &cmd, gpointer(programCommand.constData())) == 0) {
        /* no %s found so just append to it */
        g_free(cmd);
        cmd = g_strconcat(suCommand.constData(), programCommand.constData(), NULL);
      }

      GAppInfo* appInfo = g_app_info_create_from_commandline(cmd, NULL, GAppInfoCreateFlags(0), NULL);
      g_free(cmd);

      if(appInfo) {
        FmPath* cwd = page->path();
        GError* err = NULL;
        char* uri = fm_path_to_uri(cwd);
        GList* uris = g_list_prepend(NULL, uri);

        if(!g_app_info_launch_uris(appInfo, uris, NULL, &err)) {
          QMessageBox::critical(this, tr("Error"), QString::fromUtf8(err->message));
          g_error_free(err);
        }

        g_list_free(uris);
        g_free(uri);
        g_object_unref(appInfo);
      }
    }
    else {
      // show an error message and ask the user to set the command
      QMessageBox::critical(this, tr("Error"), tr("Switch user command is not set."));
      app->preferences("advanced");
    }
  }
}

void MainWindow::on_actionFindFiles_triggered() {
  Application* app = static_cast<Application*>(qApp);
  FmPathList* selectedPaths = currentPage()->selectedFilePaths();
  QStringList paths;
  if(selectedPaths) {
    for(GList* l = fm_path_list_peek_head_link(selectedPaths); l; l = l->next) {
      // FIXME: is it ok to use display name here?
      // This might be broken on filesystems with non-UTF-8 filenames.
      Fm::Path path(FM_PATH(l->data));
      paths.append(path.displayName(false));
    }
    fm_path_list_unref(selectedPaths);
  }
  else {
    paths.append(currentPage()->pathName());
  }
  app->findFiles(paths);
}

void MainWindow::on_actionOpenTerminal_triggered() {
  TabPage* page = currentPage();
  if(page) {
    Application* app = static_cast<Application*>(qApp);
    app->openFolderInTerminal(page->path());
  }
}

void MainWindow::onShortcutNextTab() {
  int current = ui.tabBar->currentIndex();
  if(current < ui.tabBar->count() - 1)
    ui.tabBar->setCurrentIndex(current + 1);
  else
    ui.tabBar->setCurrentIndex(0);
}

void MainWindow::onShortcutPrevTab() {
  int current = ui.tabBar->currentIndex();
  if(current > 0)
    ui.tabBar->setCurrentIndex(current - 1);
  else
    ui.tabBar->setCurrentIndex(ui.tabBar->count() - 1);
}

// Switch to nth tab when Alt+n or Ctrl+n is pressed
void MainWindow::onShortcutJumpToTab() {
  QShortcut* shortcut = reinterpret_cast<QShortcut*>(sender());
  QKeySequence seq = shortcut->key();
  int keyValue = seq[0];
  // See the source code of QKeySequence and refer to the method:
  // QString QKeySequencePrivate::encodeString(int key, QKeySequence::SequenceFormat format).
  // Then we know how to test if a key sequence contains a modifier.
  // It's a shame that Qt has no API for this task.

  if((keyValue & Qt::ALT) == Qt::ALT) // test if we have Alt key pressed
    keyValue -= Qt::ALT;
  else if((keyValue & Qt::CTRL) == Qt::CTRL) // test if we have Ctrl key pressed
    keyValue -= Qt::CTRL;

  // now keyValue should contains '0' - '9' only
  int index;
  if(keyValue == '0')
    index = 9;
  else
    index = keyValue - '1';
  if(index < ui.tabBar->count())
    ui.tabBar->setCurrentIndex(index);
}

}
