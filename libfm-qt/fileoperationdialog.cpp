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


#include "fileoperationdialog.h"
#include "fileoperation.h"
#include "renamedialog.h"
#include <QMessageBox>

using namespace Fm;

FileOperationDialog::FileOperationDialog(FileOperation* _operation):
  QDialog(NULL),
  operation(_operation),
  defaultOption(-1) {

  setAttribute(Qt::WA_DeleteOnClose);

  ui.setupUi(this);

  QString title;
  QString message;
  switch(_operation->type()) {
  case FM_FILE_OP_MOVE:
      title = tr("Move files");
      message = tr("Moving the following files to destination folder:");
      break;
  case FM_FILE_OP_COPY:
      title = tr("Copy Files");
      message = tr("Copying the following files to destination folder:");
      break;
  case FM_FILE_OP_TRASH:
      title = tr("Trash Files");
      message = tr("Moving the following files to trash can:");
      break;
  case FM_FILE_OP_DELETE:
      title = tr("Delete Files");
      message = tr("Deleting the following files");
      ui.dest->hide();
      ui.destLabel->hide();
      break;
  case FM_FILE_OP_LINK:
      title = tr("Create Symlinks");
      message = tr("Creating symlinks for the following files:");
      break;
  case FM_FILE_OP_CHANGE_ATTR:
      title = tr("Change Attributes");
      message = tr("Changing attributes of the following files:");
      ui.dest->hide();
      ui.destLabel->hide();
      break;
  case FM_FILE_OP_UNTRASH:
      title = tr("Restore Trashed Files");
      message = tr("Restoring the following files from trash can:");
      ui.dest->hide();
      ui.destLabel->hide();
      break;
  }
  ui.message->setText(message);
  setWindowTitle(title);
}


FileOperationDialog::~FileOperationDialog() {
}

void FileOperationDialog::setDestPath(FmPath* dest) {
  char* pathStr = fm_path_display_name(dest, false);
  ui.dest->setText(QString::fromUtf8(pathStr));
  g_free(pathStr);
}

void FileOperationDialog::setSourceFiles(FmPathList* srcFiles) {
  GList* l;
  for(l = fm_path_list_peek_head_link(srcFiles); l; l = l->next) {
    FmPath* path = FM_PATH(l->data);
    char* pathStr = fm_path_display_name(path, false);
    ui.sourceFiles->addItem(QString::fromUtf8(pathStr));
    g_free(pathStr);
  }
}

int FileOperationDialog::ask(QString question, char*const* options) {
  // TODO: implement FileOperationDialog::ask()
  return 0;
}

int FileOperationDialog::askRename(FmFileInfo* src, FmFileInfo* dest, QString& new_name) {
  int ret;
  if(defaultOption == -1) { // default action is not set, ask the user
    RenameDialog dlg(src, dest, this);
    dlg.exec();
    switch(dlg.action()) {
      case RenameDialog::ActionOverwrite:
        ret = FM_FILE_OP_OVERWRITE;
        if(dlg.applyToAll())
          defaultOption = ret;
        break;
      case RenameDialog::ActionRename:
        ret = FM_FILE_OP_RENAME;
        new_name = dlg.newName();
        break;
      case RenameDialog::ActionIgnore:
        ret = FM_FILE_OP_SKIP;
        if(dlg.applyToAll())
          defaultOption = ret;
        break;
      default:
        ret = FM_FILE_OP_CANCEL;
        break;
    }
  }
  else
    ret = defaultOption;
  return ret;
}

FmJobErrorAction FileOperationDialog::error(GError* err, FmJobErrorSeverity severity) {
  if(severity >= FM_JOB_ERROR_MODERATE) {
    QMessageBox::critical(this, tr("Error"), err->message);
    if(severity == FM_JOB_ERROR_CRITICAL)
      return FM_JOB_ABORT;
  }
  return FM_JOB_CONTINUE;
}

void FileOperationDialog::setCurFile(QString cur_file) {
  ui.curFile->setText(cur_file);
}

void FileOperationDialog::setPercent(unsigned int percent) {
  ui.progressBar->setValue(percent);
}

void FileOperationDialog::setPrepared() {
}

void FileOperationDialog::reject() {
  operation->cancel();
  QDialog::reject();
}


#include "fileoperationdialog.moc"
