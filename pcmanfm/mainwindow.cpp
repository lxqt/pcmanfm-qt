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
#include <QDir>
#include <QSettings>
#include <QDebug>

#include "tabpage.h"
#include "launcher.h"
#include <libfm-qt/filemenu.h>
#include <libfm-qt/bookmarkaction.h>
#include <libfm-qt/fileoperation.h>
#include <libfm-qt/utilities.h>
#include <libfm-qt/filepropsdialog.h>
#include <libfm-qt/pathedit.h>
#include <libfm-qt/pathbar.h>
#include <libfm-qt/path.h>
#include <libfm-qt/fileinfo.h>
#include <libfm-qt/folder.h>
#include "ui_about.h"
#include "application.h"

using namespace Fm;

namespace PCManFM {

// static
MainWindow* MainWindow::lastActive_ = nullptr;

MainWindow::MainWindow(Path path):
  QMainWindow(),
  pathEntry_(nullptr),
  pathBar_(nullptr),
  fileLauncher_(this),
  rightClickIndex_(-1),
  updatingViewMenu_(false) {

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
  if(!settings.fullWidthTabBar()) {
    ui.verticalLayout->removeWidget(ui.tabBar);
    ui.verticalLayout_2->insertWidget(0, ui.tabBar);
  }

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
  // switch to the tab under the cursor during dnd.
  ui.tabBar->setChangeCurrentOnDrag(true);
  ui.tabBar->setAcceptDrops(true);
#endif

  ui.tabBar->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.actionCloseRight, &QAction::triggered, this, &MainWindow::closeRightTabs);
  connect(ui.actionCloseLeft, &QAction::triggered, this, &MainWindow::closeLeftTabs);
  connect(ui.actionCloseOther, &QAction::triggered, this, &MainWindow::closeOtherTabs);

  connect(ui.tabBar, &QTabBar::currentChanged, this, &MainWindow::onTabBarCurrentChanged);
  connect(ui.tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::onTabBarCloseRequested);
  connect(ui.tabBar, &QTabBar::tabMoved, this, &MainWindow::onTabBarTabMoved);
  connect(ui.tabBar, &QTabBar::customContextMenuRequested, this, &MainWindow::tabContextMenu);
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

  // detect change of splitter position
  connect(ui.splitter, &QSplitter::splitterMoved, this, &MainWindow::onSplitterMoved);

  // path bar
  createPathBar(settings.pathBarButtons());
  if(settings.pathBarButtons())
    ui.actionPathButtons->setChecked(true);
  else
    ui.actionLocationBar->setChecked(true);

  // add filesystem info to status bar
  fsInfoLabel_ = new QLabel(ui.statusbar);
  ui.statusbar->addPermanentWidget(fsInfoLabel_);

  // setup the splitter
  ui.splitter->setStretchFactor(1, 1); // only the right pane can be stretched
  QList<int> sizes;
  sizes.append(settings.splitterPos());
  sizes.append(300);
  ui.splitter->setSizes(sizes);

  // load bookmark menu
  bookmarks_ = Fm::Bookmarks::dup();
  g_signal_connect(bookmarks_, "changed", G_CALLBACK(onBookmarksChanged), this);
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

  group = new QActionGroup(ui.menuPathBarStyle);
  group->setExclusive(true);
  group->addAction(ui.actionLocationBar);
  group->addAction(ui.actionPathButtons);

  // Add menubar actions to the main window this is necessary so that actions
  // shortcuts are still working when the menubar is hidden.
  addActions(ui.menubar->actions());

  // Show or hide the menu bar
  QMenu *menu = new QMenu(ui.toolBar);
  menu->addMenu(ui.menu_File);
  menu->addMenu(ui.menu_Editw);
  menu->addMenu(ui.menu_View);
  menu->addMenu(ui.menu_Go);
  menu->addMenu(ui.menu_Bookmarks);
  menu->addMenu(ui.menu_Tool);
  menu->addMenu(ui.menu_Help);
  ui.actionMenu->setMenu(menu);
  if(ui.actionMenu->icon().isNull())
    ui.actionMenu->setIcon(QIcon::fromTheme("applications-system"));
  QToolButton* menuBtn = static_cast<QToolButton*>(ui.toolBar->widgetForAction(ui.actionMenu));
  menuBtn->setPopupMode(QToolButton::InstantPopup);

  menuSep_ = ui.toolBar->insertSeparator(ui.actionMenu);
  menuSep_->setVisible(!settings.showMenuBar());
  ui.actionMenu->setVisible(!settings.showMenuBar());
  ui.menubar->setVisible(settings.showMenuBar());
  ui.actionMenu_bar->setChecked(settings.showMenuBar());
  connect(ui.actionMenu_bar, &QAction::triggered, this, &MainWindow::toggleMenuBar);

  // create shortcuts
  QShortcut* shortcut;
  shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::onResetFocus);

  shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::focusPathEntry);

  shortcut = new QShortcut(Qt::ALT + Qt::Key_D, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::focusPathEntry);

  shortcut = new QShortcut(Qt::CTRL + Qt::Key_Tab, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutNextTab);

  shortcut = new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutPrevTab);

  // Add Ctrl+PgUp and Ctrl+PgDown as well, because they are common in Firefox
  // , Opera, Google Chromium/Google Chrome and most other tab-using
  // applications.
  shortcut = new QShortcut(Qt::CTRL + Qt::Key_PageDown, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutNextTab);

  shortcut = new QShortcut(Qt::CTRL + Qt::Key_PageUp, this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutPrevTab);

  int i;
  for(i = 0; i < 10; ++i) {
    shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_0 + i), this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutJumpToTab);

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_0 + i), this);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::onShortcutJumpToTab);
  }

  shortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionGoUp_triggered);

  shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionDelete_triggered);

  shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_I), this);
  connect(shortcut, &QShortcut::activated, this, &MainWindow::focusFilterBar);

  // in addition to F3, for convenience
  shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), this);
  connect(shortcut, &QShortcut::activated, ui.actionFindFiles, &QAction::trigger);

  if(QToolButton* clearButton = ui.filterBar->findChild<QToolButton*>()) {
    clearButton->setToolTip(tr("Clear text (Ctrl+K)"));
    shortcut = new QShortcut(Qt::CTRL + Qt::Key_K, this);
    connect(shortcut, &QShortcut::activated, ui.filterBar, &QLineEdit::clear);
  }

  if(!path.isNull())
    addTab(path);

  // size from settings
  if(settings.rememberWindowSize()) {
    resize(settings.windowWidth(), settings.windowHeight());
    if(settings.windowMaximized())
      setWindowState(windowState() | Qt::WindowMaximized);
  }

  if(QApplication::layoutDirection() == Qt::RightToLeft)
    setRTLIcons(true);
}

MainWindow::~MainWindow() {
  if(!bookmarks_.isNull()) {
    g_signal_handlers_disconnect_by_func(bookmarks_, (gpointer)G_CALLBACK(onBookmarksChanged), this);
  }
}

void MainWindow::chdir(Path path) {
  TabPage* page = currentPage();

  if(page) {
    ui.filterBar->clear();
    page->chdir(path, true);
    updateUIForCurrentPage();
  }
}

void MainWindow::createPathBar(bool usePathButtons) {
  QWidget* bar;
  if(usePathButtons) {
    bar = pathBar_ = new Fm::PathBar(this);
    connect(pathBar_, &Fm::PathBar::chdir, this, &MainWindow::onPathBarChdir);
    connect(pathBar_, &Fm::PathBar::middleClickChdir, this, &MainWindow::onPathBarMiddleClickChdir);
    connect(pathBar_, &Fm::PathBar::editingFinished, this, &MainWindow::onResetFocus);
  }
  else {
    bar = pathEntry_ = new Fm::PathEdit(this);
    connect(pathEntry_, &Fm::PathEdit::returnPressed, this, &MainWindow::onPathEntryReturnPressed);
    connect(pathEntry_, &QLineEdit::textEdited, this, &MainWindow::onPathEntryEdited);
  }
  ui.toolBar->insertWidget(ui.actionGo, bar);
  ui.actionGo->setVisible(!usePathButtons);
}

// add a new tab
int MainWindow::addTab(Path path) {
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

  return index;
}

void MainWindow::toggleMenuBar(bool checked) {
  Settings& settings = static_cast<Application*>(qApp)->settings();
  bool showMenuBar = !settings.showMenuBar();

  if (!showMenuBar) {
    if (QMessageBox::Cancel == QMessageBox::warning(this,
          tr("Hide menu bar"),
          tr("This will hide the menu bar completely, use Ctrl+M to show it again."),
          QMessageBox::Ok | QMessageBox::Cancel)) {
      ui.actionMenu_bar->setChecked(true);
      return;
    }
  }

  ui.menubar->setVisible(showMenuBar);
  ui.actionMenu_bar->setChecked(showMenuBar);
  menuSep_->setVisible(!showMenuBar);
  ui.actionMenu->setVisible(!showMenuBar);
  settings.setShowMenuBar(showMenuBar);
}

void MainWindow::onPathEntryReturnPressed() {
  QString text = pathEntry_->text();
  QByteArray utext = text.toUtf8();
  chdir(Fm::Path::newForDisplayName(utext));
}

void MainWindow::onPathEntryEdited(const QString& text) {
  QString realText(text);
  if(realText == "~" || realText.startsWith("~/")) {
    realText.replace(0, 1, QDir::homePath());
    pathEntry_->setText(realText);
  }
}

void MainWindow::onPathBarChdir(FmPath* dirPath) {
  // call chdir() only when needed because otherwise
  // filter bar will be cleard on changing current tab
  TabPage* page = currentPage();
  if(page && dirPath != page->path())
    chdir(dirPath);
}

void MainWindow::onPathBarMiddleClickChdir(FmPath* dirPath) {
  addTab(dirPath);
}

void MainWindow::on_actionGoUp_triggered() {
  TabPage* page = currentPage();

  if(page) {
    ui.filterBar->clear();
    page->up();
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
  chdir(Fm::Path::getHome());
}

void MainWindow::on_actionReload_triggered() {
  currentPage()->reload();
  if(pathEntry_ != nullptr)
    pathEntry_->setText(currentPage()->pathName());
}

void MainWindow::on_actionConnectToServer_triggered() {
  Application* app = static_cast<Application*>(qApp);
  app->connectToServer();
}

void MainWindow::on_actionGo_triggered() {
  onPathEntryReturnPressed();
}

void MainWindow::on_actionNewTab_triggered() {
  Fm::Path path = currentPage()->path();
  int index = addTab(path);
  ui.tabBar->setCurrentIndex(index);
}

void MainWindow::on_actionNewWin_triggered() {
  Fm::Path path = currentPage()->path();
  (new MainWindow(path))->show();
}

void MainWindow::on_actionNewFolder_triggered() {
  if(TabPage* tabPage = currentPage()) {
    Fm::Path dirPath = tabPage->folderView()->path();

    if(dirPath)
      createFileOrFolder(CreateNewFolder, dirPath);
  }
}

void MainWindow::on_actionNewBlankFile_triggered() {
  if(TabPage* tabPage = currentPage()) {
    Fm::Path dirPath = tabPage->folderView()->path();

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

void MainWindow::on_actionFileProperties_triggered() {
  TabPage* page = currentPage();
  if(page) {
    Fm::FileInfoList files = page->selectedFiles();
    if(!files.isNull()) {
      Fm::FilePropsDialog::showForFiles(files);
    }
  }
}

void MainWindow::on_actionFolderProperties_triggered() {
  TabPage* page = currentPage();

  if(page) {
    Fm::Folder folder = page->folder();
    if(!folder.isNull()) {
      Fm::FileInfo info = folder.getInfo();
      if(!info.isNull())
        Fm::FilePropsDialog::showForFile(info);
    }
  }
}

void MainWindow::on_actionShowHidden_triggered(bool checked) {
  currentPage()->setShowHidden(checked);
  ui.sidePane->setShowHidden(checked);
}

void MainWindow::on_actionByFileName_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileName, currentPage()->sortOrder());
}

void MainWindow::on_actionByMTime_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileMTime, currentPage()->sortOrder());
}

void MainWindow::on_actionByOwner_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileOwner, currentPage()->sortOrder());
}

void MainWindow::on_actionByFileSize_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileSize, currentPage()->sortOrder());
}

void MainWindow::on_actionByFileType_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileType, currentPage()->sortOrder());
}

void MainWindow::on_actionAscending_triggered(bool checked) {
  currentPage()->sort(currentPage()->sortColumn(), Qt::AscendingOrder);
}

void MainWindow::on_actionDescending_triggered(bool checked) {
  currentPage()->sort(currentPage()->sortColumn(), Qt::DescendingOrder);
}

void MainWindow::on_actionCaseSensitive_triggered(bool checked) {
  currentPage()->setSortCaseSensitive(checked);
}

void MainWindow::on_actionFolderFirst_triggered(bool checked) {
  currentPage()->setSortFolderFirst(checked);
}

void MainWindow::on_actionPreserveView_triggered(bool checked) {
  TabPage* page = currentPage();
  page->setCustomizedView(!page->hasCustomizedView());
}

void MainWindow::on_actionFilter_triggered(bool checked) {
  ui.filterBar->setVisible(checked);
  if(checked)
    ui.filterBar->setFocus();
  else if(TabPage* tabPage = currentPage()) {
    ui.filterBar->clear();
    tabPage->folderView()->childView()->setFocus();
    // clear filter string for all tabs
    int n = ui.stackedWidget->count();
    for(int i = 0; i < n; ++i) {
      TabPage* page = static_cast<TabPage*>(ui.stackedWidget->widget(i));
      if(!page->getFilterStr().isEmpty()) {
        page->setFilterStr(QString());
        page->applyFilter();
      }
    }
  }
  static_cast<Application*>(qApp)->settings().setShowFilter(checked);
}

void MainWindow::on_actionLocationBar_triggered(bool checked) {
  if(checked) {
    // show current path in a location bar entry
    if(pathBar_ != nullptr) {
      delete pathBar_;
      pathBar_ = nullptr;
    }
    createPathBar(false);
    pathEntry_->setText(currentPage()->pathName());
    static_cast<Application*>(qApp)->settings().setPathBarButtons(false);
  }
}

void MainWindow::on_actionPathButtons_triggered(bool checked) {
  if(checked && pathBar_ == nullptr) {
    // show current path as buttons
    if(pathEntry_ != nullptr) {
      delete pathEntry_;
      pathEntry_ = nullptr;
    }
    createPathBar(true);
    pathBar_->setPath(currentPage()->path());
    static_cast<Application*>(qApp)->settings().setPathBarButtons(true);
  }
}

void MainWindow::on_actionComputer_triggered() {
  chdir(Fm::Path::newForUri("computer:///"));
}

void MainWindow::on_actionApplications_triggered() {
  chdir(Fm::Path::getAppsMenu());
}

void MainWindow::on_actionTrash_triggered() {
  chdir(Fm::Path::getTrash());
}

void MainWindow::on_actionNetwork_triggered() {
  chdir(Fm::Path::newForUri("network:///"));
}

void MainWindow::on_actionDesktop_triggered() {
  chdir(Fm::Path::getDesktop());
}

void MainWindow::on_actionAddToBookmarks_triggered() {
  TabPage* page = currentPage();
  if(page) {
    Fm::Path cwd = page->path();
    if(!cwd.isNull()) {
      char* dispName = cwd.displayBasename();
      bookmarks_.insert(cwd, dispName, -1);
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
      ui.version->setText(tr("Version: %1").arg(PCMANFM_QT_VERSION));
    }
  private:
    Ui::AboutDialog ui;
  };
  AboutDialog dialog(this);
  dialog.exec();
}

void MainWindow::on_actionIconView_triggered() {
  currentPage()->setViewMode(Fm::FolderView::IconMode);
}

void MainWindow::on_actionCompactView_triggered() {
  currentPage()->setViewMode(Fm::FolderView::CompactMode);
}

void MainWindow::on_actionDetailedList_triggered() {
  currentPage()->setViewMode(Fm::FolderView::DetailedListMode);
}

void MainWindow::on_actionThumbnailView_triggered() {
  currentPage()->setViewMode(Fm::FolderView::ThumbnailMode);
}

void MainWindow::onTabBarCloseRequested(int index) {
  closeTab(index);
}

void MainWindow::onResetFocus() {
  if(TabPage* page = currentPage()) {
    page->folderView()->childView()->setFocus();
  }
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

void MainWindow::focusFilterBar() {
  if(!ui.filterBar->isVisible())
    ui.actionFilter->trigger();
  else
    ui.filterBar->setFocus();
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
  if(settings.rememberWindowSize()) {
    settings.setLastWindowMaximized(isMaximized());

    if(!isMaximized()) {
        settings.setLastWindowWidth(width());
        settings.setLastWindowHeight(height());
    }
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  if(lastActive_ == this)
    lastActive_ = nullptr;

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
  fsInfoLabel_->setText(text);
  fsInfoLabel_->setVisible(!text.isEmpty());
}

void MainWindow::updateViewMenuForCurrentPage() {
  if(updatingViewMenu_)  // prevent recursive calls
    return;
  updatingViewMenu_ = true;
  TabPage* tabPage = currentPage();
  if(tabPage) {
    // update menus. FIXME: should we move this to another method?
    ui.actionShowHidden->setChecked(tabPage->showHidden());
    ui.actionPreserveView->setChecked(tabPage->hasCustomizedView());

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
  updatingViewMenu_ = false;
}

void MainWindow::updateUIForCurrentPage() {
  TabPage* tabPage = currentPage();

  if(tabPage) {
    setWindowTitle(tabPage->title());
    if(pathEntry_ != nullptr) {
      pathEntry_->setText(tabPage->pathName());
    }
    else if(pathBar_ != nullptr) {
      pathBar_->setPath(tabPage->path());
    }
    ui.statusbar->showMessage(tabPage->statusText());
    fsInfoLabel_->setText(tabPage->statusText(TabPage::StatusTextFSInfo));
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

void MainWindow::onTabPageStatusChanged(int type, QString statusText) {
  TabPage* tabPage = static_cast<TabPage*>(sender());
  if(tabPage == currentPage()) {
    switch(type) {
    case TabPage::StatusTextNormal:
    case TabPage::StatusTextSelectedFiles: {
      // FIXME: updating the status text so frequently is a little bit ineffiecient
      QString text = statusText = tabPage->statusText(TabPage::StatusTextSelectedFiles);
      if(text.isEmpty())
        ui.statusbar->showMessage(tabPage->statusText(TabPage::StatusTextNormal));
      else
        ui.statusbar->showMessage(text);
      break;
    }
    case TabPage::StatusTextFSInfo:
      fsInfoLabel_->setText(tabPage->statusText(TabPage::StatusTextFSInfo));
      fsInfoLabel_->setVisible(!statusText.isEmpty());
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

  case OpenInNewWindow:
    (new MainWindow(path))->show();
    break;
  }
}

void MainWindow::onTabPageSortFilterChanged() {
  TabPage* tabPage = static_cast<TabPage*>(sender());
  if(tabPage == currentPage()) {
    updateViewMenuForCurrentPage();
    if(!tabPage->hasCustomizedView()) {
      Settings& settings = static_cast<Application*>(qApp)->settings();
      settings.setSortColumn(static_cast<Fm::FolderModel::ColumnId>(tabPage->sortColumn()));
      settings.setSortOrder(tabPage->sortOrder());
      settings.setSortFolderFirst(tabPage->sortFolderFirst());
      settings.setSortCaseSensitive(tabPage->sortCaseSensitive());
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
  GList* allBookmarks = bookmarks_.getAll();
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
  QList<QAction*> actions = pThis->ui.menu_Bookmarks->actions();
  // there are 2 items after the separator on the bookmarks menu.
  // loadBookmarksMenu() will add the separator too.
  QList<QAction*>::const_iterator it = actions.constBegin();
  QList<QAction*>::const_iterator last_it = actions.constEnd() - 2;

  // Check for an invalid last_it.
  Q_ASSERT(it <= last_it);
  if (it > last_it) {
    qWarning() << Q_FUNC_INFO << QString::fromLatin1("Warning: it > last_it");
    return;
  }

  while(it != last_it) {
    QAction* action = *it;
    ++it;
    pThis->ui.menu_Bookmarks->removeAction(action);
  }

  pThis->loadBookmarksMenu();
}

void MainWindow::onBookmarkActionTriggered() {
  BookmarkAction* action = static_cast<BookmarkAction*>(sender());
  Fm::Path path = action->path();
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
  Fm::PathList paths = page->selectedFilePaths();
  copyFilesToClipboard(paths);
}

void MainWindow::on_actionCut_triggered() {
  TabPage* page = currentPage();
  Fm::PathList paths = page->selectedFilePaths();
  cutFilesToClipboard(paths);
}

void MainWindow::on_actionPaste_triggered() {
  pasteFilesFromClipboard(currentPage()->path(), this);
}

void MainWindow::on_actionDelete_triggered() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  TabPage* page = currentPage();
  Fm::PathList paths = page->selectedFilePaths();

  bool shiftPressed = (qApp->keyboardModifiers() & Qt::ShiftModifier ? true : false);
  if(settings.useTrash() && !shiftPressed)
    FileOperation::trashFiles(paths, settings.confirmTrash(), this);
  else
    FileOperation::deleteFiles(paths, settings.confirmDelete(), this);
}

void MainWindow::on_actionRename_triggered() {
  TabPage* page = currentPage();
  Fm::FileInfoList files = page->selectedFiles();
  for(GList* l = fm_file_info_list_peek_head_link(files); l; l = l->next) {
    FmFileInfo* file = FM_FILE_INFO(l->data);
    Fm::renameFile(file, NULL);
  }
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

// change some icons according to layout direction
void MainWindow::setRTLIcons(bool isRTL) {
  QIcon nxtIcn = QIcon::fromTheme("go-next");
  QIcon prevIcn = QIcon::fromTheme("go-previous");
  if(isRTL) {
    ui.actionGoBack->setIcon(nxtIcn);
    ui.actionCloseLeft->setIcon(nxtIcn);
    ui.actionGoForward->setIcon(prevIcn);
    ui.actionCloseRight->setIcon(prevIcn);
  }
  else {
    ui.actionGoBack->setIcon(prevIcn);
    ui.actionCloseLeft->setIcon(prevIcn);
    ui.actionGoForward->setIcon(nxtIcn);
    ui.actionCloseRight->setIcon(nxtIcn);
  }
}

bool MainWindow::event(QEvent* event) {
  switch(event->type()) {
  case QEvent::WindowActivate:
    lastActive_ = this;
  default:
    break;
  }
  return QMainWindow::event(event);
}

void MainWindow::changeEvent(QEvent *event) {
  switch(event->type()) {
    case QEvent::LayoutDirectionChange:
      setRTLIcons(QApplication::layoutDirection() == Qt::RightToLeft);
      break;
    default:
      break;
  }
  QWidget::changeEvent(event);
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
    char* name = path.displayName(true);
    QAction* action = menu.addAction(QString::fromUtf8(name));
    g_free(name);
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

void MainWindow::tabContextMenu(const QPoint& pos) {
  int tabNum = ui.tabBar->count();
  if(tabNum <= 1) return;

  rightClickIndex_ = ui.tabBar->tabAt(pos);
  if(rightClickIndex_ < 0) return;

  QMenu menu;
  if(rightClickIndex_ > 0)
      menu.addAction(ui.actionCloseLeft);
  if(rightClickIndex_ < tabNum - 1) {
    menu.addAction(ui.actionCloseRight);
    if(rightClickIndex_ > 0) {
      menu.addSeparator();
      menu.addAction(ui.actionCloseOther);
    }
  }
  menu.exec(ui.tabBar->mapToGlobal(pos));
}

void MainWindow::closeLeftTabs() {
  while(rightClickIndex_ > 0) {
    closeTab(rightClickIndex_ - 1);
    --rightClickIndex_;
  }
}

void MainWindow::closeRightTabs() {
  if(rightClickIndex_ < 0) return;
  while(rightClickIndex_ < ui.tabBar->count() - 1)
    closeTab(rightClickIndex_ + 1);
}

void MainWindow::focusPathEntry() {
  // use text entry for the path bar
  if(pathEntry_ != nullptr) {
    pathEntry_->setFocus();
    pathEntry_->selectAll();
  }
  else if (pathBar_ != nullptr) { // use button-style path bar
    pathBar_->openEditor();
  }
}

void MainWindow::updateFromSettings(Settings& settings) {
  // apply settings

  // menu
  ui.actionDelete->setText(settings.useTrash() ? tr("&Move to Trash") : tr("&Delete"));
  ui.actionDelete->setIcon(settings.useTrash() ? QIcon::fromTheme("user-trash") : QIcon::fromTheme("edit-delete"));

  // side pane
  ui.sidePane->setIconSize(QSize(settings.sidePaneIconSize(), settings.sidePaneIconSize()));

  // tabs
  ui.tabBar->setTabsClosable(settings.showTabClose());
  ui.tabBar->setVisible(settings.alwaysShowTabs() || (ui.tabBar->count() > 1));
  if(ui.verticalLayout->indexOf(ui.tabBar) > -1) {
    if(!settings.fullWidthTabBar()) {
      ui.verticalLayout->removeWidget(ui.tabBar);
      ui.verticalLayout_2->insertWidget(0, ui.tabBar);
    }
  }
  else if (ui.verticalLayout_2->indexOf(ui.tabBar) > -1 && settings.fullWidthTabBar()) {
    ui.verticalLayout_2->removeWidget(ui.tabBar);
    ui.verticalLayout->insertWidget(0, ui.tabBar);
  }

  // all tab pages
  int n = ui.stackedWidget->count();

  for(int i = 0; i < n; ++i) {
    TabPage* page = static_cast<TabPage*>(ui.stackedWidget->widget(i));
    page->updateFromSettings(settings);
  }
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
      // FIXME: it's better to get the filename of the current process rather than hard-code pcmanfm-qt here.
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
        Fm::Path cwd = page->path();
        GError* err = NULL;
        char* uri = cwd.toUri();
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
  Fm::PathList selectedPaths = currentPage()->selectedFilePaths();
  QStringList paths;
  if(!selectedPaths.isNull()) {
    for(GList* l = fm_path_list_peek_head_link(selectedPaths); l; l = l->next) {
      // FIXME: is it ok to use display name here?
      // This might be broken on filesystems with non-UTF-8 filenames.
      Fm::Path path(FM_PATH(l->data));
      paths.append(path.displayName(false));
    }
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
