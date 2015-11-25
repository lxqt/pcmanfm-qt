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


#ifndef FM_TABPAGE_H
#define FM_TABPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <libfm/fm.h>
#include <libfm-qt/browsehistory.h>
#include "view.h"
#include <libfm-qt/path.h>

namespace Fm {
  class FileLauncher;
  class FolderModel;
  class ProxyFolderModel;
  class CachedFolderModel;
};

namespace PCManFM {

class Settings;
class Launcher;

class ProxyFilter : public Fm::ProxyFolderModelFilter {
public:
  bool filterAcceptsRow(const Fm::ProxyFolderModel* model, FmFileInfo* info) const;
  virtual ~ProxyFilter() {}
  void setVirtHidden(FmFolder* folder);
  QString getFilterStr() {
    return filterStr_;
  }
  void setFilterStr(QString str) {
    filterStr_ = str;
  }

private:
  QString filterStr_;
  QStringList virtHiddenList_;
};

class TabPage : public QWidget {
Q_OBJECT

public:
  enum StatusTextType {
    StatusTextNormal,
    StatusTextSelectedFiles,
    StatusTextFSInfo,
    StatusTextNum
  };

public:
  explicit TabPage(FmPath* path, QWidget* parent = nullptr);
  virtual ~TabPage();

  void chdir(FmPath* newPath, bool addHistory = true);

  Fm::FolderView::ViewMode viewMode() {
    return folderView_->viewMode();
  }

  void setViewMode(Fm::FolderView::ViewMode mode) {
    folderView_->setViewMode(mode);
  }

  void sort(int col, Qt::SortOrder order = Qt::AscendingOrder) {
    // if(folderModel_)
    //  folderModel_->sort(col, order);
    if(proxyModel_)
      proxyModel_->sort(col, order);
  }

  int sortColumn() {
    return proxyModel_->sortColumn();
  }

  Qt::SortOrder sortOrder() {
    return proxyModel_->sortOrder();
  }

  bool sortFolderFirst() {
    return proxyModel_->folderFirst();
  }
  void setSortFolderFirst(bool value) {
    proxyModel_->setFolderFirst(value);
  }

  bool sortCaseSensitive() {
    return proxyModel_->sortCaseSensitivity();
  }
  void setSortCaseSensitive(bool value) {
    proxyModel_->setSortCaseSensitivity(value ? Qt::CaseSensitive : Qt::CaseInsensitive);
  }

  bool showHidden() {
    return proxyModel_->showHidden();
  }

  void setShowHidden(bool showHidden);

  FmPath* path() {
    return folder_ ? fm_folder_get_path(folder_) : nullptr;
  }

  QString pathName();

  FmFolder* folder() {
    return folder_;
  }

  Fm::FolderModel* folderModel() {
    return reinterpret_cast<Fm::FolderModel*>(folderModel_);
  }

  View* folderView() {
    return folderView_;
  }

  Fm::BrowseHistory& browseHistory() {
    return history_;
  }

  FmFileInfoList* selectedFiles() {
    return folderView_->selectedFiles();
  }

  FmPathList* selectedFilePaths() {
    return folderView_->selectedFilePaths();
  }

  void selectAll();

  void invertSelection();

  void reload() {
    if(folder_) {
      proxyFilter_->setVirtHidden(folder_); // reread ".hidden"
      fm_folder_reload(folder_);
    }
  }

  QString title() const {
    return title_;
  }

  QString statusText(StatusTextType type = StatusTextNormal) const {
    return statusText_[type];
  }

  bool canBackward() {
    return history_.canBackward();
  }

  void backward();

  bool canForward() {
    return history_.canForward();
  }

  void forward();

  void jumpToHistory(int index);

  bool canUp();

  void up();

  void updateFromSettings(Settings& settings);

  void setFileLauncher(Fm::FileLauncher* launcher) {
    folderView_->setFileLauncher(launcher);
  }

  Fm::FileLauncher* fileLauncher() {
    return folderView_->fileLauncher();
  }

  QString getFilterStr() {
    if(proxyFilter_)
      return proxyFilter_->getFilterStr();
    return QString();
  }

  void setFilterStr(QString str) {
    if(proxyFilter_)
      proxyFilter_->setFilterStr(str);
  }

  void applyFilter();

Q_SIGNALS:
  void statusChanged(int type, QString statusText);
  void titleChanged(QString title);
  void openDirRequested(FmPath* path, int target);
  void sortFilterChanged();
  void forwardRequested();
  void backwardRequested();

protected Q_SLOTS:
  void onOpenDirRequested(FmPath* path, int target);
  void onModelSortFilterChanged();
  void onSelChanged(int numSel);
  void restoreScrollPos();

private:
  void freeFolder();
  QString formatStatusText();

  static void onFolderStartLoading(FmFolder* _folder, TabPage* pThis);
  static void onFolderFinishLoading(FmFolder* _folder, TabPage* pThis);
  static FmJobErrorAction onFolderError(FmFolder* _folder, GError* err, FmJobErrorSeverity severity, TabPage* pThis);
  static void onFolderFsInfo(FmFolder* _folder, TabPage* pThis);
  static void onFolderRemoved(FmFolder* _folder, TabPage* pThis);
  static void onFolderUnmount(FmFolder* _folder, TabPage* pThis);
  static void onFolderContentChanged(FmFolder* _folder, TabPage* pThis);

private:
  View* folderView_;
  Fm::CachedFolderModel* folderModel_;
  Fm::ProxyFolderModel* proxyModel_;
  ProxyFilter* proxyFilter_;
  QVBoxLayout* verticalLayout;
  FmFolder* folder_;
  QString title_;
  QString statusText_[StatusTextNum];
  Fm::BrowseHistory history_; // browsing history
  Fm::Path lastFolderPath_; // last browsed folder
  bool overrideCursor_;
};

}

#endif // FM_TABPAGE_H
