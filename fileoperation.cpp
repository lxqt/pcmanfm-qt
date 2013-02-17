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


#include "fileoperation.h"
#include "fileoperationdialog.h"
#include <QTimer>

using namespace Fm;

#define SHOW_DLG_DELAY  1000

FileOperation::FileOperation(Type type, FmPathList* srcFiles, QObject* parent): QObject(parent) {
  job = fm_file_ops_job_new((FmFileOpType)type, srcFiles);

  g_signal_connect(job, "ask", G_CALLBACK(onFileOpsJobAsk), this);
  g_signal_connect(job, "ask-rename", G_CALLBACK(onFileOpsJobAskRename), this);
  g_signal_connect(job, "error", G_CALLBACK(onFileOpsJobError), this);
  g_signal_connect(job, "prepared", G_CALLBACK(onFileOpsJobPrepared), this);
  g_signal_connect(job, "cur-file", G_CALLBACK(onFileOpsJobCurFile), this);
  g_signal_connect(job, "percent", G_CALLBACK(onFileOpsJobPercent), this);
  g_signal_connect(job, "finished", G_CALLBACK(onFileOpsJobFinished), this);
  g_signal_connect(job, "cancelled", G_CALLBACK(onFileOpsJobCancelled), this);
}

void FileOperation::disconnectJob() {
  g_signal_handlers_disconnect_by_func(job, (gpointer)G_CALLBACK(onFileOpsJobAsk), this);
  g_signal_handlers_disconnect_by_func(job, (gpointer)G_CALLBACK(onFileOpsJobAskRename), this);
  g_signal_handlers_disconnect_by_func(job, (gpointer)G_CALLBACK(onFileOpsJobError), this);
  g_signal_handlers_disconnect_by_func(job, (gpointer)G_CALLBACK(onFileOpsJobPrepared), this);
  g_signal_handlers_disconnect_by_func(job, (gpointer)G_CALLBACK(onFileOpsJobCurFile), this);
  g_signal_handlers_disconnect_by_func(job, (gpointer)G_CALLBACK(onFileOpsJobPercent), this);
  g_signal_handlers_disconnect_by_func(job, (gpointer)G_CALLBACK(onFileOpsJobFinished), this);
  g_signal_handlers_disconnect_by_func(job, (gpointer)G_CALLBACK(onFileOpsJobCancelled), this);
}

FileOperation::~FileOperation() {
  if(showUITimer) {
    showUITimer->stop();
    delete showUITimer;
    showUITimer = NULL;
  }

  if(job) {
    disconnectJob();
    g_object_unref(job);
  }
}

bool FileOperation::run() {
  // run the job
  showUITimer = new QTimer();
  showUITimer->setSingleShot(true);
  showUITimer->start(SHOW_DLG_DELAY);
  connect(showUITimer, SIGNAL(timeout()), SLOT(onShowDialogTimeout()));

  fm_job_run_async(FM_JOB(job));
}

void FileOperation::onShowDialogTimeout() {
  showDialog();
}

void FileOperation::showDialog() {
  if(!dlg) {
    // disable the timer
    if(showUITimer) {
      showUITimer->stop();
      delete showUITimer;
      showUITimer = NULL;
    }

    dlg = new FileOperationDialog();
    dlg->show();
  }
}

gint FileOperation::onFileOpsJobAsk(FmFileOpsJob* job, const char* question, char*const* options, FileOperation* pThis) {

}

gint FileOperation::onFileOpsJobAskRename(FmFileOpsJob* job, FmFileInfo* src, FmFileInfo* dest, char** new_name, FileOperation* pThis) {

}

void FileOperation::onFileOpsJobCancelled(FmFileOpsJob* job, FileOperation* pThis) {
  qDebug("file operation is cancelled!");
}

void FileOperation::onFileOpsJobCurFile(FmFileOpsJob* job, const char* cur_file, FileOperation* pThis) {

}

FmJobErrorAction FileOperation::onFileOpsJobError(FmFileOpsJob* job, GError* err, FmJobErrorSeverity severity, FileOperation* pThis) {

}

void FileOperation::onFileOpsJobFinished(FmFileOpsJob* job, FileOperation* pThis) {

}

void FileOperation::onFileOpsJobPercent(FmFileOpsJob* job, guint percent, FileOperation* pThis) {
  if(pThis->dlg) {
  }
}

void FileOperation::onFileOpsJobPrepared(FmFileOpsJob* job, FileOperation* pThis) {

}

#include "fileoperation.moc"