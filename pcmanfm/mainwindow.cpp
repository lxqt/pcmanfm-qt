#include "mainwindow.h"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QSplitter>

#include "tabpage.h"
#include "filelauncher.h"
#include "filemenu.h"
#include "bookmarkaction.h"
#include "fileoperation.h"
#include "utilities.h"
#include "ui_about.h"
#include "application.h"

// #include "qmodeltest/modeltest.h"

using namespace Fm;

namespace PCManFM {

MainWindow::MainWindow(FmPath* path):
  QMainWindow() {

  // setup user interface
  ui.setupUi(this);

  // tabbed browsing interface
  ui.tabBar->setDocumentMode(true);
  ui.tabBar->setTabsClosable(true);
  ui.tabBar->setElideMode(Qt::ElideRight);
  ui.tabBar->setExpanding(false);
  connect(ui.tabBar, SIGNAL(currentChanged(int)), SLOT(onTabBarCurrentChanged(int)));
  connect(ui.tabBar, SIGNAL(tabCloseRequested(int)), SLOT(onTabBarCloseRequested(int)));
  connect(ui.stackedWidget, SIGNAL(widgetRemoved(int)), SLOT(onStackedWidgetWidgetRemoved(int)));

  // side pane
  connect(ui.sidePane, SIGNAL(chdirRequested(int,FmPath*)), SLOT(onSidePaneChdirRequested(int,FmPath*)));
  
  // path bar
  pathEntry = new QLineEdit(this);
  connect(pathEntry, SIGNAL(returnPressed()), SLOT(onPathEntryReturnPressed()));
  ui.toolBar->insertWidget(ui.actionGo, pathEntry);

  // add filesystem info to status bar
  fsInfoLabel = new QLabel(ui.statusbar);
  ui.statusbar->addPermanentWidget(fsInfoLabel);

  // setup the splitter
  ui.splitter->setStretchFactor(1, 1); // only the right pane can be stretched
  QList<int> sizes;
  sizes.append(150);
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
  
  
  if(path)
    addTab(path);
}

MainWindow::~MainWindow() {
  if(bookmarks)
    g_object_unref(bookmarks);
}

void MainWindow::chdir(FmPath* path) {
  TabPage* page = currentPage();
  if(page) {
    page->chdir(path, true);
    updateUIForCurrentPage();
  }
}

// add a new tab
void MainWindow::addTab(FmPath* path) {
  TabPage* newPage = new TabPage(path, this);
  int index = ui.stackedWidget->addWidget(newPage);
  connect(newPage, SIGNAL(titleChanged(QString)), SLOT(onTabPageTitleChanged(QString)));
  connect(newPage, SIGNAL(statusChanged(int,QString)), SLOT(onTabPageStatusChanged(int,QString)));
  connect(newPage, SIGNAL(openDirRequested(FmPath*,int)), SLOT(onTabPageOpenDirRequested(FmPath*,int)));
  
  ui.tabBar->insertTab(index, newPage->title());
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
    page->up();
    updateUIForCurrentPage();
  }
}

void MainWindow::on_actionGoBack_triggered() {
  TabPage* page = currentPage();
  if(page) {
    page->backward();
    updateUIForCurrentPage();
  }
}

void MainWindow::on_actionGoForward_triggered() {
  TabPage* page = currentPage();
  if(page) {
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
  MainWindow* newWin = new MainWindow(path);
  newWin->show();
}

void MainWindow::on_actionShowHidden_triggered(bool checked) {
  TabPage* tabPage = currentPage();
  tabPage->setShowHidden(checked);
}

void MainWindow::on_actionByFileName_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileName);
}

void MainWindow::on_actionByMTime_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileMTime);
}

void MainWindow::on_actionByOwner_triggered(bool checked) {
}

void MainWindow::on_actionByFileType_triggered(bool checked) {
}

void MainWindow::on_actionAscending_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileName, Qt::AscendingOrder);
}

void MainWindow::on_actionDescending_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnFileName, Qt::DescendingOrder);
}


void MainWindow::on_actionComputer_triggered() {
  FmPath* path = fm_path_new_for_uri("computer:///");
  chdir(path);
  fm_path_unref(path);
}

void MainWindow::on_actionApplications_triggered() {
  chdir(fm_path_get_apps_menu());
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
  
}

void MainWindow::on_actionQuit_triggered() {
  QApplication::quit();
}

void MainWindow::on_actionAbout_triggered() {
  // the about dialog
  class AboutDialog : public QDialog {
  public:
    explicit AboutDialog(QWidget* parent = 0, Qt::WindowFlags f = 0) {
      ui.setupUi(this);
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
  if(ui.tabBar->count() == 1) { // this is the last one
    close(); // destroy the whole window
  }
  else {
    QWidget* page = ui.stackedWidget->widget(index);
    ui.stackedWidget->removeWidget(page);
    // NOTE: we do not remove the tab here.
    // it'll be donoe in onStackedWidgetWidgetRemoved()
    // notebook->removeTab(index);
  }
}

void MainWindow::onTabBarCurrentChanged(int index) {
  ui.stackedWidget->setCurrentIndex(index);
  updateUIForCurrentPage();
}

void MainWindow::updateUIForCurrentPage() {
  TabPage* tabPage = currentPage();
  if(tabPage) {
    setWindowTitle(tabPage->title());
    pathEntry->setText(tabPage->pathName());
    ui.statusbar->showMessage(tabPage->statusText());
    fsInfoLabel->setText(tabPage->statusText(TabPage::StatusTextFSInfo));
    tabPage->folderView()->childView()->setFocus();

    // update back/forward/up toolbar buttons
    ui.actionGoUp->setEnabled(tabPage->canUp());
    ui.actionGoBack->setEnabled(tabPage->canBackward());
    ui.actionGoForward->setEnabled(tabPage->canForward());

    // update menus
  }
}


void MainWindow::onStackedWidgetWidgetRemoved(int index) {
  // need to remove associated tab from tabBar
  ui.tabBar->removeTab(index);
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
	ui.statusbar->showMessage(statusText);
	break;
      case TabPage::StatusTextFSInfo:
	fsInfoLabel->setText(tabPage->statusText(TabPage::StatusTextFSInfo));
	break;
      case TabPage::StatusTextSelectedFiles:
	break;
    }
  }
}

void MainWindow::onTabPageOpenDirRequested(FmPath* path, int target) {
  switch(target) {
    case View::OpenInCurrentView:
      chdir(path);
      break;
    case View::OpenInNewTab:
      addTab(path);
      break;
    case View::OpenInNewWindow: {
      Application* app = static_cast<Application*>(qApp);
      MainWindow* newWin = new MainWindow(path);
      // TODO: apply window size from app->settings
      newWin->resize(640, 480);
      newWin->show();
      break;
    }
  }
}

void MainWindow::onSidePaneChdirRequested(int type, FmPath* path) {
  chdir(path);
}

void MainWindow::loadBookmarksMenu() {
  GList* l = fm_bookmarks_get_all(bookmarks);
  QAction* before = ui.actionAddToBookmarks;
  for(; l; l = l->next) {
    FmBookmarkItem* item = reinterpret_cast<FmBookmarkItem*>(l->data);
    BookmarkAction* action = new BookmarkAction(item);
    connect(action, SIGNAL(triggered(bool)), SLOT(onBookmarkActionTriggered()));
    ui.menu_Bookmarks->insertAction(before, action);
  }
  ui.menu_Bookmarks->insertSeparator(before);
}

void MainWindow::onBookmarksChanged(FmBookmarks* bookmarks, MainWindow* pThis) {
  // delete existing items
  QList<QAction*> actions = pThis->ui.menu_Bookmarks->actions();
  QList<QAction*>::const_iterator it = actions.begin();
  QList<QAction*>::const_iterator last_it = actions.end() - 1;
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
  if(path)
    chdir(path);
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

void MainWindow::on_actionDelete_triggered() {
  TabPage* page = currentPage();
  FmPathList* paths = page->selectedFilePaths();
  FileOperation::deleteFiles(paths, this);
  fm_path_list_unref(paths);
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

}

#include "mainwindow.moc"
