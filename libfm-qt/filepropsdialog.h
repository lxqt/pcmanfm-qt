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


#ifndef FM_FILEPROPSDIALOG_H
#define FM_FILEPROPSDIALOG_H

#include "libfmqtglobals.h"
#include <QDialog>
#include <QTimer>
#include <libfm/fm.h>

namespace Ui {
  class FilePropsDialog;
};

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
    FilePropsDialog* dlg = showForFiles(files, parent);
    fm_file_info_list_unref(files);
    return dlg;
  }
  
  static FilePropsDialog* showForFiles(FmFileInfoList* files, QWidget* parent = 0) {
    FilePropsDialog* dlg = new FilePropsDialog(files, parent);
    dlg->show();
    return dlg;
  }

private:
  void initGeneralPage();
  void initApplications();
  void initPermissionsPage();
  void initOwner();

  static void onDeepCountJobFinished(FmDeepCountJob* job, FilePropsDialog* pThis);

private Q_SLOTS:
  void onFileSizeTimerTimeout();
  
private:
  Ui::FilePropsDialog* ui;
  FmFileInfoList* fileInfos_; // list of all file infos
  FmFileInfo* fileInfo; // file info of the first file in the list
  bool singleType; // all files are of the same type?
  bool singleFile; // only one file is selected?
  bool hasDir; // is there any dir in the files?
  bool allNative; // all files are on native UNIX filesystems (not virtual or remote)

  FmMimeType* mimeType; // mime type of the files

  gint32 uid; // owner uid of the files, -1 means all files do not have the same uid
  gint32 gid; // owner gid of the files, -1 means all files do not have the same uid
  mode_t ownerPerm; // read permission of the files, -1 means not all files have the same value
  int ownerPermSel;
  mode_t groupPerm; // read permission of the files, -1 means not all files have the same value
  int groupPermSel;
  mode_t otherPerm; // read permission of the files, -1 means not all files have the same value
  int otherPermSel;
  mode_t execPerm; // exec permission of the files
  Qt::CheckState execCheckState;

  FmDeepCountJob* deepCountJob; // job used to count total size
  QTimer* fileSizeTimer;
};

}

#endif // FM_FILEPROPSDIALOG_H
