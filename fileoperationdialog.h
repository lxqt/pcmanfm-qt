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


#ifndef FM_FILEOPERATIONDIALOG_H
#define FM_FILEOPERATIONDIALOG_H

#include <QDialog>
#include "ui_file-operation-dialog.h"

namespace Fm {

class FileOperationDialog : public QDialog
{
Q_OBJECT
public:
  explicit FileOperationDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
  virtual ~FileOperationDialog();
  
private:
  Ui::FileOperationDialog ui;
};

}

#endif // FM_FILEOPERATIONDIALOG_H
