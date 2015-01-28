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


#include "mountoperationquestiondialog_p.h"
#include "mountoperation.h"
#include <QPushButton>

namespace Fm {

MountOperationQuestionDialog::MountOperationQuestionDialog(MountOperation* op, gchar* message, GStrv choices):
  QMessageBox(),
  mountOperation(op) {

  setIcon(QMessageBox::Question);
  setText(QString::fromUtf8(message));

  choiceCount = g_strv_length(choices);
  choiceButtons = new QAbstractButton*[choiceCount];
  for(int i = 0; i < choiceCount; ++i) {
    // It's not allowed to add custom buttons without standard roles
    // to QMessageBox. So we set role of all buttons to AcceptRole and
    // handle their clicked() signals in our own slots.
    // When anyone of the buttons is clicked, exec() always returns "accept".
    QPushButton* button = new QPushButton(QString::fromUtf8(choices[i]));
    addButton(button, QMessageBox::AcceptRole);
    choiceButtons[i] = button;
  }
  connect(this, &MountOperationQuestionDialog::buttonClicked, this, &MountOperationQuestionDialog::onButtonClicked);
}

MountOperationQuestionDialog::~MountOperationQuestionDialog() {
  delete []choiceButtons;
}

void MountOperationQuestionDialog::done(int r) {
  if(r != QDialog::Accepted) {
    GMountOperation* op = mountOperation->mountOperation();
    g_mount_operation_reply(op, G_MOUNT_OPERATION_ABORTED);
  }
  QDialog::done(r);
}

void MountOperationQuestionDialog::onButtonClicked(QAbstractButton* button) {
  GMountOperation* op = mountOperation->mountOperation();
  for(int i = 0; i < choiceCount; ++i) {
    if(choiceButtons[i] == button) {
      g_mount_operation_set_choice(op, i);
      g_mount_operation_reply(op, G_MOUNT_OPERATION_HANDLED);
      break;
    }
  }
}

} // namespace Fm
