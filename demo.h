#ifndef demo_H
#define demo_H

#include "ui_main-win.h"
#include <QtGui/QMainWindow>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <libfm/fm.h>
#include "foldermodel.h"
#include "proxyfoldermodel.h"
#include "folderview.h"

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    MainWindow();
    virtual ~MainWindow();

    void chdir(FmPath* path);

private:
  Ui::MainWindow ui;
  QLineEdit* pathEntry;
  Fm::FolderView* view;
  Fm::FolderModel* model;
  Fm::ProxyFolderModel* sortModel;
  FmFolder* folder;

protected Q_SLOTS:
  void onViewClicked(int type, FmFileInfo* file);

  void on_actionUP_triggered();
  void on_actionHome_triggered();
  void on_actionReload_triggered();

  void on_actionIconView_triggered();
  void on_actionCompactView_triggered();
  void on_actionDetailedList_triggered();
  void on_actionThumbnailView_triggered();

  void on_actionGo_triggered();
  void on_actionNew_triggered();
  void on_actionShowHidden_triggered(bool check);
  void on_actionApplications_triggered();
  void on_actionComputer_triggered();
  void on_actionTrash_triggered();
  void on_actionNetwork_triggered();
  void on_actionDesktop_triggered();
  void on_actionAddToBookmarks_triggered();
  void on_actionQuit_triggered();
  void on_actionAbout_triggered();
};

#endif // demo_H
