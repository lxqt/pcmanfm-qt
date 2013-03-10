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


#ifndef FM_RENAMEDIALOG_H
#define FM_RENAMEDIALOG_H

#include <QDialog>
#include <libfm/fm.h>

namespace Ui {
  class RenameDialog;
};

class QAbstractButton;

namespace Fm {

class LIBFM_QT_API RenameDialog : public QDialog {
Q_OBJECT

public:
  enum Action {
    ActionCancel,
    ActionRename,
    ActionOverwrite,
    ActionIgnore
  };

public:
  explicit RenameDialog(FmFileInfo* src, FmFileInfo* dest, QWidget* parent = 0, Qt::WindowFlags f = 0);
  virtual ~RenameDialog();

  Action action() {
    return action_;
  }

  bool applyToAll() {
    return applyToAll_;
  }
  
  QString newName() {
    return newName_;
  }

protected Q_SLOTS:
  void onRenameClicked();
  void onIgnoreClicked();
  void onFileNameChanged(QString newName);

protected:
  void accept();
  void reject();

private:
  Ui::RenameDialog* ui;
  QAbstractButton* renameButton_;
  Action action_;
  bool applyToAll_;
  QString oldName_;
  QString newName_;
};

}

#endif // FM_RENAMEDIALOG_H
