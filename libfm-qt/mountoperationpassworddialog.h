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


#ifndef FM_MOUNTOPERATIONPASSWORDDIALOG_H
#define FM_MOUNTOPERATIONPASSWORDDIALOG_H

#include <QDialog>
#include <gio/gio.h>

namespace Ui {
  class MountOperationPasswordDialog;
};

namespace Fm {
  
class MountOperation;

class MountOperationPasswordDialog : public QDialog {
Q_OBJECT

public:
  explicit MountOperationPasswordDialog(MountOperation* op, GAskPasswordFlags flags);
  virtual ~MountOperationPasswordDialog();

  void setMessage(QString message);
  void setDefaultUser(QString user);
  void setDefaultDomain(QString domain);

  virtual void done(int r);

private Q_SLOTS:
  void onAnonymousToggled(bool checked);
  
private:
  Ui::MountOperationPasswordDialog* ui;
  MountOperation* mountOperation;
  bool needPassword;
  bool needUserName;
  bool needDomain;
  bool canSavePassword;
  bool canAnonymous;
};

}

#endif // FM_MOUNTOPERATIONPASSWORDDIALOG_H
