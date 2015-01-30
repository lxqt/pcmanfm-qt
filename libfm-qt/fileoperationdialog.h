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


#ifndef FM_FILEOPERATIONDIALOG_H
#define FM_FILEOPERATIONDIALOG_H

#include "libfmqtglobals.h"
#include <QDialog>
#include <libfm/fm.h>

namespace Ui {
  class FileOperationDialog;
};

namespace Fm {

class FileOperation;

class LIBFM_QT_API FileOperationDialog : public QDialog {
Q_OBJECT
public:
  explicit FileOperationDialog(FileOperation* _operation);
  virtual ~FileOperationDialog();
  
  void setSourceFiles(FmPathList* srcFiles);
  void setDestPath(FmPath* dest);

  int ask(QString question, char* const* options);
  int askRename(FmFileInfo* src, FmFileInfo* dest, QString& new_name);
  FmJobErrorAction error(GError* err, FmJobErrorSeverity severity);
  void setPrepared();
  void setCurFile(QString cur_file);
  void setPercent(unsigned int percent);
  void setRemainingTime(unsigned int sec);

  virtual void reject();

private:
  Ui::FileOperationDialog* ui;
  FileOperation* operation;
  int defaultOption;
};

}

#endif // FM_FILEOPERATIONDIALOG_H
