#include "mainwindow.h"

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QtGui/QVBoxLayout>
#include <QMessageBox>
#include <QSplitter>

#include "tabpage.h"
#include "filelauncher.h"
#include "filemenu.h"

// #include "qmodeltest/modeltest.h"

using namespace Fm;

MainWindow::MainWindow(FmPath* path) {
  ui.setupUi(this);

  // tabbed browsing interface
  ui.tabBar->setDocumentMode(true);
  ui.tabBar->setTabsClosable(true);
  ui.tabBar->setElideMode(Qt::ElideRight);
  ui.tabBar->setExpanding(false);
  connect(ui.tabBar, SIGNAL(currentChanged(int)), SLOT(onTabBarCurrentChanged(int)));
  connect(ui.tabBar, SIGNAL(tabCloseRequested(int)), SLOT(onTabBarCloseRequested(int)));
  connect(ui.stackedWidget, SIGNAL(widgetRemoved(int)), SLOT(onStackedWidgetWidgetRemoved(int)));

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

  // open home dir by default
  addTab(path ? path : fm_path_get_home());
}

MainWindow::~MainWindow() {
}

void MainWindow::chdir(FmPath* path) {
  TabPage* page = currentPage();
  if(page) {
    page->chdir(path);
    pathEntry->setText(page->pathName());
  }
}

// add a new tab
void MainWindow::addTab(FmPath* path) {
  TabPage* newPage = new TabPage(path, this);
  int index = ui.stackedWidget->addWidget(newPage);
  connect(newPage, SIGNAL(titleChanged(QString)), SLOT(onTabPageTitleChanged(QString)));
  connect(newPage, SIGNAL(statusChanged(int,QString)), SLOT(onTabPageStatusChanged(int,QString)));
  connect(newPage, SIGNAL(fileClicked(int,FmFileInfo*)), SLOT(onTabPageFileClicked(int,FmFileInfo*)));

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
  FmPath* path = currentPage()->path();
  if(path) {
    FmPath* parent = fm_path_get_parent(path);
    if(parent)
      chdir(parent);
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

}

void MainWindow::on_actionByFileName_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnName);
}

void MainWindow::on_actionByMTime_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnMTime);
}

void MainWindow::on_actionByOwner_triggered(bool checked) {
}

void MainWindow::on_actionByFileType_triggered(bool checked) {
}

void MainWindow::on_actionAscending_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnName, Qt::AscendingOrder);
}

void MainWindow::on_actionDescending_triggered(bool checked) {
  currentPage()->sort(Fm::FolderModel::ColumnName, Qt::DescendingOrder);
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
  QMessageBox::about(this, QString::fromUtf8("PCManFM"), QString::fromUtf8("The Qt port of PCManFM.\nCreated by PCMan"));
}

void MainWindow::on_actionIconView_triggered() {
  qDebug("HERE\n");
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
  qDebug("tab closed");
  if(ui.tabBar->count() == 1) { // this is the last one
    destroy(); // destroy the whole window
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
  qDebug("current changed");
  ui.stackedWidget->setCurrentIndex(index);
  TabPage* tabPage = currentPage();
  if(tabPage) {
    setWindowTitle(tabPage->title());
    pathEntry->setText(tabPage->pathName());
    ui.statusbar->showMessage(tabPage->statusText());
    fsInfoLabel->setText(tabPage->statusText(TabPage::StatusTextFSInfo));
  }
}

void MainWindow::onStackedWidgetWidgetRemoved(int index) {
  // need to remove associated tab from tabBar
  ui.tabBar->removeTab(index);
}

void MainWindow::onTabPageTitleChanged(QString title) {
  TabPage* tabPage = reinterpret_cast<TabPage*>(sender());
  int index = ui.stackedWidget->indexOf(tabPage);
  qDebug("index = %d\n", index);
  if(index >= 0)
    ui.tabBar->setTabText(index, title);

  if(tabPage == currentPage())
    setWindowTitle(title);
}

void MainWindow::onTabPageStatusChanged(int type, QString statusText) {
  TabPage* tabPage = reinterpret_cast<TabPage*>(sender());
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

void MainWindow::onTabPageFileClicked(int type, FmFileInfo* fileInfo) {
  if(type == Fm::FolderView::ActivatedClick) {
    if(fm_file_info_is_dir(fileInfo)) {
      chdir(fm_file_info_get_path(fileInfo));
    }
    else {
      GList* files = g_list_append(NULL, fileInfo);
      Fm::FileLauncher::launch(NULL, files);
      g_list_free(files);
    }
  }
  else if(type == Fm::FolderView::ContextMenuClick) {
    FmFolder* folder = currentPage()->folder();
    // show context menu
    // FmFileInfoList* files = view->selectedFiles();
    FmFileInfoList* files = fm_file_info_list_new();
    fm_file_info_list_push_tail(files, fileInfo);
    Fm::FileMenu* menu = new Fm::FileMenu(files, fileInfo, fm_folder_get_path(folder));
    fm_file_info_list_unref(files);
    menu->popup(QCursor::pos());
    connect(menu, SIGNAL(aboutToHide()),SLOT(onPopupMenuHide()));
  }
}

void MainWindow::onPopupMenuHide() {
  Fm::FileMenu* menu = (Fm::FileMenu*)sender();
  //delete the menu;
  menu->deleteLater();
}

#include "mainwindow.moc"
