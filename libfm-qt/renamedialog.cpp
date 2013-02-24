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


#include "renamedialog.h"
#include <QStringBuilder>
#include "icontheme.h"

using namespace Fm;

RenameDialog::RenameDialog(FmFileInfo* src, FmFileInfo* dest, QWidget* parent, Qt::WindowFlags f):
  QDialog(parent, f) {

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
  g_free(basename);
}

RenameDialog::~RenameDialog() {

}

#include "renamedialog.moc"
