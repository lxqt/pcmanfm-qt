#ifndef FM_MAIN_WINDOW_H
#define FM_MAIN_WINDOW_H

#include "ui_main-win.h"
#include <QtGui/QMainWindow>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QTabWidget>
#include <libfm/fm.h>
#include "foldermodel.h"
#include "folderview.h"
#include "placesview.h"
#include <QMessageBox>
#include <QTabBar>
#include <QStackedWidget>
#include <QSplitter>

namespace Fm {

class TabPage;

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
  MainWindow(FmPath* path = NULL);
  virtual ~MainWindow();

  void chdir(FmPath* path);
  void addTab(FmPath* path);

  TabPage* currentPage() {
    return reinterpret_cast<TabPage*>(ui.stackedWidget->currentWidget());
  }

private:
  Ui::MainWindow ui;
  QLineEdit* pathEntry;
  QLabel* fsInfoLabel;

protected Q_SLOTS:

  void onPathEntryReturnPressed();

  void on_actionNewTab_triggered();
  void on_actionNewWin_triggered();
  void on_actionQuit_triggered();

  void on_actionGoUp_triggered();
  void on_actionHome_triggered();
  void on_actionReload_triggered();

  void on_actionIconView_triggered();
  void on_actionCompactView_triggered();
  void on_actionDetailedList_triggered();
  void on_actionThumbnailView_triggered();

  void on_actionGo_triggered();
  void on_actionShowHidden_triggered(bool check);

  void on_actionByFileName_triggered(bool checked);
  void on_actionByMTime_triggered(bool checked);
  void on_actionByOwner_triggered(bool checked);
  void on_actionByFileType_triggered(bool checked);
  void on_actionAscending_triggered(bool checked);
  void on_actionDescending_triggered(bool checked);
  
  void on_actionApplications_triggered();
  void on_actionComputer_triggered();
  void on_actionTrash_triggered();
  void on_actionNetwork_triggered();
  void on_actionDesktop_triggered();
  void on_actionAddToBookmarks_triggered();
  void on_actionAbout_triggered();

  void onTabBarCloseRequested(int index);
  void onTabBarCurrentChanged(int index);
  
  void onStackedWidgetWidgetRemoved(int index);
  
  void onTabPageTitleChanged(QString title);
  void onTabPageStatusChanged(int type, QString statusText);
  void onTabPageFileClicked(int type, FmFileInfo* fileInfo);
  void onPopupMenuHide();

};

}

#endif // FM_MAIN_WINDOW_H
