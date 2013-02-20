/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

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
#include "folderview.h"
#include "foldermodel.h"
#include "placesview.h"
#include "proxyfoldermodel.h"

namespace PCManFM {

class TabPage : public QWidget
{
Q_OBJECT

public:
  enum StatusTextType {
    StatusTextNormal,
    StatusTextSelectedFiles,
    StatusTextFSInfo,
    StatusTextNum
  };

public:
  explicit TabPage(FmPath* path, QWidget* parent = 0);
  virtual ~TabPage();

  void chdir(FmPath* path);

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

  bool showHidden() {
    return showHidden_;
  }
  
  void setShowHidden(bool showHidden) {
    showHidden_ = showHidden;
    proxyModel_->setShowHidden(showHidden_);
  }
  
  FmPath* path() {
    return folder_ ? fm_folder_get_path(folder_) : NULL;
  }

  QString pathName();
  
  FmFolder* folder() {
    return folder_;
  }

  Fm::FolderModel* folderModel() {
    return folderModel_;
  }

  Fm::FolderView* folderView() {
    return folderView_;
  }

  FmFileInfoList* selectedFiles() {
    return folderView_->selectedFiles();
  }

  FmPathList* selectedFilePaths() {
    return folderView_->selectedFilePaths();
  }
  
  void reload() {
    if(folder_)
      fm_folder_reload(folder_);
  }

  QString title() const {
    return title_;
  }
  
  QString statusText(StatusTextType type = StatusTextNormal) const {
    return statusText_[type];
  }
  
Q_SIGNALS:
  void statusChanged(int type, QString statusText);
  void fileClicked(int type, FmFileInfo* file);
  void titleChanged(QString title);

protected Q_SLOTS:
  void onViewClicked(int type, FmFileInfo* file);

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
  Fm::FolderView* folderView_;
  Fm::FolderModel* folderModel_;
  Fm::ProxyFolderModel* proxyModel_;
  QVBoxLayout* verticalLayout;
  FmFolder* folder_;
  QString title_;
  QString statusText_[StatusTextNum];
  bool showHidden_;
};

}

#endif // FM_TABPAGE_H
