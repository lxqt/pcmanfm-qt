/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "execfiledialog_p.h"
#include "ui_exec-file.h"
#include "icontheme.h"

namespace Fm {

ExecFileDialog::ExecFileDialog(FmFileInfo* file, QWidget* parent, Qt::WindowFlags f):
  QDialog (parent, f),
  fileInfo_(fm_file_info_ref(file)),
  result_(FM_FILE_LAUNCHER_EXEC_CANCEL),
  ui(new Ui::ExecFileDialog()) {

  ui->setupUi(this);
  // show file icon
  FmIcon* icon = fm_file_info_get_icon(fileInfo_);
  ui->icon->setPixmap(IconTheme::icon(icon).pixmap(QSize(48, 48)));

  QString msg;
  if(fm_file_info_is_text(file)) {
    msg = tr("This text file '%1' seems to be an executable script.\nWhat do you want to do with it?")
            .arg(QString::fromUtf8(fm_file_info_get_disp_name(file)));
    ui->execTerm->setDefault(true);
  }
  else {
    msg= tr("This file '%1' is executable. Do you want to execute it?")
           .arg(QString::fromUtf8(fm_file_info_get_disp_name(file)));
    ui->exec->setDefault(true);
    ui->open->hide();
  }
  ui->msg->setText(msg);
}

ExecFileDialog::~ExecFileDialog() {
  delete ui;
  if(fileInfo_)
    fm_file_info_unref(fileInfo_);
}

void ExecFileDialog::accept() {
  QObject* _sender = sender();
  if(_sender == ui->exec)
    result_ = FM_FILE_LAUNCHER_EXEC;
  else if(_sender == ui->execTerm)
    result_ = FM_FILE_LAUNCHER_EXEC_IN_TERMINAL;
  else if(_sender == ui->open)
    result_ = FM_FILE_LAUNCHER_EXEC_OPEN;
  QDialog::accept();
}

} // namespace Fm
