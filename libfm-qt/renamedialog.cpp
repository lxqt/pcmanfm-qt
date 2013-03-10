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


#include "renamedialog.h"
#include <QStringBuilder>
#include <QPushButton>
#include "icontheme.h"

using namespace Fm;

RenameDialog::RenameDialog(FmFileInfo* src, FmFileInfo* dest, QWidget* parent, Qt::WindowFlags f):
  QDialog(parent, f),
  action_(ActionIgnore),
  applyToAll_(false) {

  ui.setupUi(this);

  FmPath* path = fm_file_info_get_path(dest);
  FmIcon* srcIcon = fm_file_info_get_icon(src);
  FmIcon* destIcon = fm_file_info_get_icon(dest);

  // show info for the source file
  QIcon icon = IconTheme::icon(srcIcon);
  QSize iconSize(fm_config->big_icon_size, fm_config->big_icon_size);
  QPixmap pixmap = icon.pixmap(iconSize);
  ui.srcIcon->setPixmap(pixmap);

  QString infoStr;
  const char* disp_size = fm_file_info_get_disp_size(src);
  if(disp_size) {
    infoStr = QString(tr("Type: %1\nSize: %2\nModified: %3"))
                .arg(QString::fromUtf8(fm_file_info_get_desc(src)))
                .arg(QString::fromUtf8(disp_size))
                .arg(QString::fromUtf8(fm_file_info_get_disp_mtime(src)));
  }
  else {
    infoStr = QString(tr("Type: %1\nModified: %3"))
                .arg(QString::fromUtf8(fm_file_info_get_desc(src)))
                .arg(QString::fromUtf8(fm_file_info_get_disp_mtime(src)));
  }
  ui.srcInfo->setText(infoStr);

  // show info for the dest file
  icon = IconTheme::icon(destIcon);
  pixmap = icon.pixmap(iconSize);
  ui.destIcon->setPixmap(pixmap);

  disp_size = fm_file_info_get_disp_size(dest);
  if(disp_size) {
    infoStr = QString(tr("Type: %1\nSize: %2\nModified: %3"))
                .arg(QString::fromUtf8(fm_file_info_get_desc(dest)))
                .arg(QString::fromUtf8(disp_size))
                .arg(QString::fromUtf8(fm_file_info_get_disp_mtime(dest)));
  }
  else {
    infoStr = QString(tr("Type: %1\nModified: %3"))
                .arg(QString::fromUtf8(fm_file_info_get_desc(src)))
                .arg(QString::fromUtf8(fm_file_info_get_disp_mtime(src)));
  }
  ui.destInfo->setText(infoStr);

  char* basename = fm_path_display_basename(path);
  ui.fileName->setText(QString::fromUtf8(basename));
  oldName_ = basename;
  g_free(basename);
  connect(ui.fileName, SIGNAL(textChanged(QString)), SLOT(onFileNameChanged(QString)));

  // add "Rename" button
  QAbstractButton* button = ui.buttonBox->button(QDialogButtonBox::Ok);
  button->setText(tr("&Overwrite"));
  // FIXME: there seems to be no way to place the Rename button next to the overwrite one.
  renameButton_ = ui.buttonBox->addButton(tr("&Rename"), QDialogButtonBox::ActionRole);
  connect(renameButton_, SIGNAL(clicked(bool)), SLOT(onRenameClicked()));
  renameButton_->setEnabled(false); // disabled by default

  button = ui.buttonBox->button(QDialogButtonBox::Ignore);
  connect(button, SIGNAL(clicked(bool)), SLOT(onIgnoreClicked()));
}

RenameDialog::~RenameDialog() {

}

void RenameDialog::onRenameClicked() {
  action_ = ActionRename;
  QDialog::done(QDialog::Accepted);
}

void RenameDialog::onIgnoreClicked() {
  action_ = ActionIgnore;
}

// the overwrite button
void RenameDialog::accept() {
  action_ = ActionOverwrite;
  applyToAll_ = ui.applyToAll->isChecked();
  QDialog::accept();
}

// cancel, or close the dialog
void RenameDialog::reject() {
  action_ = ActionCancel;
  QDialog::reject();
}

void RenameDialog::onFileNameChanged(QString newName) {
  newName_ = newName;
  // FIXME: check if the name already exists in the current dir
  renameButton_->setEnabled((newName_ != oldName_));
}


#include "renamedialog.moc"
