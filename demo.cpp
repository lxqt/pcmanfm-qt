#include "demo.h"

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QtGui/QVBoxLayout>
#include <QMessageBox>

#include "proxyfoldermodel.h"
#include "filelauncher.h"
#include "qmodeltest/modeltest.h"

MainWindow::MainWindow():
  // new ModelTest(model, this);

  folder (NULL) {
  // path bar
  ui.setupUi(this);

  pathEntry = new QLineEdit(this);
  ui.toolBar->insertWidget(ui.actionGo, pathEntry);
  view = new Fm::FolderView(this);
  view->setIconSize(QSize(32, 32));
  view->setColumnWidth(Fm::FolderModel::ColumnName, 200);
  // view->setWrapping(true);
  // view->setWordWrap(true);
  // view->setViewMode(QListView::IconMode);
  // view->setViewMode(QListView::ListMode);
  setCentralWidget(view);
  connect(view, SIGNAL(clicked(int, FmFileInfo*)), SLOT(onViewClicked(int, FmFileInfo*)));

  model = new Fm::FolderModel();
  Fm::ProxyFolderModel* sortModel = new Fm::ProxyFolderModel();
  sortModel->setSourceModel(model);
  sortModel->setDynamicSortFilter(true);
  sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
  sortModel->setSortLocaleAware(true);
  sortModel->sort(0, Qt::AscendingOrder);
  sortModel->setShowHidden(false);
  view->setModel(sortModel);
  chdir(fm_path_get_home());
  view->setFocus();
}

MainWindow::~MainWindow() {
  if(folder)
    g_object_unref(folder);
  if(model)
    delete model;
}

void MainWindow::chdir(FmPath* path) {
  if(folder) {
    if(fm_path_equal(path, fm_folder_get_path(folder)))
      return;
    g_object_unref(folder);
  }
  folder = fm_folder_from_path(path);
  model->setFolder(folder);
  char* disp_path = fm_path_display_name(path, FALSE);
  pathEntry->setText(QString::fromUtf8(disp_path));
  g_free(disp_path);
}

void MainWindow::on_actionUP_triggered() {
  FmPath* path = folder ? fm_folder_get_path(folder) : NULL;
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
  if(folder)
    fm_folder_reload(folder);
}

void MainWindow::on_actionGo_triggered() {
  QString text = pathEntry->text();
  QByteArray utext = text.toUtf8();
  FmPath* path = fm_path_new_for_display_name(utext);
  chdir(path);
  fm_path_unref(path);
}

void MainWindow::on_actionNew_triggered() {
  
}

void MainWindow::on_actionShowHidden_triggered(bool checked) {
  sortModel->setShowHidden(checked);
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
  QMessageBox::about(this, QString::fromUtf8("Libfm Demo"), QString::fromUtf8("The Qt4 port of libfm.\nCreated by PCMan"));
}

void MainWindow::onViewClicked(int type, FmFileInfo* file) {
  if(type == Fm::FolderView::ACTIVATED) {
    if(fm_file_info_is_dir(file)) {
      chdir(fm_file_info_get_path(file));
    }
    else {
      GList* files = g_list_append(NULL, file);
      Fm::FileLauncher::launch(NULL, files);
      g_list_free(files);
    }
  }
  else if(type == Fm::FolderView::CONTEXT_MENU) {
    
  }
}


#include "demo.moc"
