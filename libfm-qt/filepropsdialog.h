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


#ifndef FM_FILEPROPSDIALOG_H
#define FM_FILEPROPSDIALOG_H

#include <QDialog>
#include <QTimer>
#include "ui_file-props.h"
#include <libfm/fm.h>

namespace Fm {

class LIBFM_QT_API FilePropsDialog : public QDialog {
Q_OBJECT

public:
  explicit FilePropsDialog(FmFileInfoList* files, QWidget* parent = 0, Qt::WindowFlags f = 0);
  virtual ~FilePropsDialog();

  virtual void accept();
  
  static FilePropsDialog* showForFile(FmFileInfo* file, QWidget* parent = 0) {
    FmFileInfoList* files = fm_file_info_list_new();
    fm_file_info_list_push_tail(files, file);
    showForFiles(files, parent);
    fm_file_info_list_unref(files);
  }
  
  static FilePropsDialog* showForFiles(FmFileInfoList* files, QWidget* parent = 0) {
    FilePropsDialog* dlg = new FilePropsDialog(files, parent);
    dlg->show();
  }

private:
  void initGeneral();
  void initApplications();
  void initPermissions();

  static void onDeepCountJobFinished(FmDeepCountJob* job, FilePropsDialog* pThis);

private Q_SLOTS:
  void onFileSizeTimerTimeout();
  
private:
  Ui::FilePropsDialog ui;
  FmFileInfoList* fileInfos_;
  FmFileInfo* fileInfo;
  bool singleType;
  bool singleFile;
  bool allNative;
  bool hasDir;
  bool allDirs;
  FmMimeType* mimeType;
  GList* appInfos;
  GAppInfo* defaultApp;

  gint32 uid; // owner uid
  gint32 gid; // owner group

  FmDeepCountJob* deepCountJob; // job used to count total size
  QTimer* fileSizeTimer;
};

}

#endif // FM_FILEPROPSDIALOG_H
