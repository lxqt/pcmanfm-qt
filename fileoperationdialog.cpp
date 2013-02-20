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

using namespace Fm;

FileOperationDialog::FileOperationDialog(FileOperation* _operation):
  QDialog(NULL),
  operation(_operation) {

  ui.setupUi(this);
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

  return 0;
}

int FileOperationDialog::askRename(FmFileInfo* src, FmFileInfo* dest, QString& new_name) {
  return 0;
}

FmJobErrorAction FileOperationDialog::error(GError* err, FmJobErrorSeverity severity) {

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
