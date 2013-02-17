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


#include "filepropsdialog.h"
#include "icontheme.h"

using namespace Fm;

FilePropsDialog::FilePropsDialog(FmFileInfoList* files, QWidget* parent, Qt::WindowFlags f):
  QDialog(parent, f),
  fileInfos_(fm_file_info_list_ref(files)),
  singleType(fm_file_info_list_is_same_type(files)),
  singleFile(fm_file_info_list_get_length(files) == 1 ? true:false),
  fileInfo(fm_file_info_list_peek_head(files)),
  mimeType(NULL) {

  ui.setupUi(this);

  if(singleType) {
    mimeType = fm_mime_type_ref(fm_file_info_get_mime_type(fileInfo));
  }

  FmPathList* paths = fm_path_list_new_from_file_info_list(files);
  deepCountJob = fm_deep_count_job_new(paths, FM_DC_JOB_DEFAULT);
  fm_path_list_unref(paths);    

  /*
  data->timeout = g_timeout_add(600, on_timeout, data);
  g_signal_connect(dlg, "response", G_CALLBACK(on_response), data);
  g_signal_connect_swapped(dlg, "destroy", G_CALLBACK(fm_file_prop_data_free), data);
  g_signal_connect(data->dc_job, "finished", G_CALLBACK(on_finished), data);

  fm_job_run_async(FM_JOB(data->dc_job));
  */
  initGeneral();
  initApplications();
  initPermissions();
    
}

FilePropsDialog::~FilePropsDialog() {
  if(fileInfos_)
    fm_file_info_list_unref(fileInfos_);
}

void FilePropsDialog::initApplications() {

}

void FilePropsDialog::initPermissions() {

}

void FilePropsDialog::initGeneral() {
  // update UI
  if(singleType) { // all files are of the same mime-type
    FmIcon* icon = NULL;
    // FIXME: handle custom icons for some files
    // FIXME: display special property pages for special files or
    // some specified mime-types.
    if(singleFile) { // only one file is selected.
      icon = fm_file_info_get_icon(fileInfo);
    }
    if(mimeType) {
      if(!icon) // get an icon from mime type if needed
        icon = fm_mime_type_get_icon(mimeType);
      ui.fileType->setText(QString::fromUtf8(fm_mime_type_get_desc(mimeType)));
      ui.mimeType->setText(QString::fromUtf8(fm_mime_type_get_type(mimeType)));
    }
    if(icon) {
      ui.iconButton->setIcon(IconTheme::icon(icon));
    }

    if(singleFile && fm_file_info_is_symlink(fileInfo)) {
      ui.target->setText(QString::fromUtf8(fm_file_info_get_target(fileInfo)));
    }
    else {
      ui.target->hide();
      ui.targetLabel->hide();
    }
  } // end if(singleType)
  else { // not singleType, multiple files are selected at the same time
    ui.fileType->setText(tr("Files of different types"));
    ui.target->hide();
    ui.targetLabel->hide();
  }

  // FIXME: check if all files has the same parent dir, mtime, or atime
  if(singleFile) { // only one file is selected
    char buf[128];
    FmPath* parent_path = fm_path_get_parent(fm_file_info_get_path(fileInfo));
    char* parent_str = parent_path ? fm_path_display_name(parent_path, TRUE) : NULL;
    time_t atime;
    struct tm tm;

    ui.fileName->setText(QString::fromUtf8(fm_file_info_get_disp_name(fileInfo)));
    if(parent_str) {
      ui.location->setText(QString::fromUtf8(parent_str));
      g_free(parent_str);
    }
    else
      ui.location->clear();

    ui.lastModified->setText(QString::fromUtf8(fm_file_info_get_disp_mtime(fileInfo)));

    /* FIXME: need to encapsulate this in an libfm API. */
    /*
    atime = fm_file_info_get_atime(fileInfo);
    localtime_r(&atime, &tm);
    strftime(buf, sizeof(buf), "%x %R", &tm);
    gtk_label_set_text(data->atime, buf);
    */
  }
  else {
    ui.fileName->setText(tr("Multiple Files"));
    ui.fileName->setEnabled(false);
  }
  // on_timeout(data);
}

#include "filepropsdialog.moc"
