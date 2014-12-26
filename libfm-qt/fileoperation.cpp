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


#include "fileoperation.h"
#include "fileoperationdialog.h"
#include <QTimer>
#include <QMessageBox>

using namespace Fm;

#define SHOW_DLG_DELAY  1000

FileOperation::FileOperation(Type type, FmPathList* srcFiles, QObject* parent):
  QObject(parent),
  dlg(NULL),
  destPath(NULL),
  srcPaths(fm_path_list_ref(srcFiles)),
  autoDestroy_(true),
  job_(fm_file_ops_job_new((FmFileOpType)type, srcFiles)) {

  g_signal_connect(job_, "ask", G_CALLBACK(onFileOpsJobAsk), this);
  g_signal_connect(job_, "ask-rename", G_CALLBACK(onFileOpsJobAskRename), this);
  g_signal_connect(job_, "error", G_CALLBACK(onFileOpsJobError), this);
  g_signal_connect(job_, "prepared", G_CALLBACK(onFileOpsJobPrepared), this);
  g_signal_connect(job_, "cur-file", G_CALLBACK(onFileOpsJobCurFile), this);
  g_signal_connect(job_, "percent", G_CALLBACK(onFileOpsJobPercent), this);
  g_signal_connect(job_, "finished", G_CALLBACK(onFileOpsJobFinished), this);
  g_signal_connect(job_, "cancelled", G_CALLBACK(onFileOpsJobCancelled), this);
}

void FileOperation::disconnectJob() {
  g_signal_handlers_disconnect_by_func(job_, (gpointer)G_CALLBACK(onFileOpsJobAsk), this);
  g_signal_handlers_disconnect_by_func(job_, (gpointer)G_CALLBACK(onFileOpsJobAskRename), this);
  g_signal_handlers_disconnect_by_func(job_, (gpointer)G_CALLBACK(onFileOpsJobError), this);
  g_signal_handlers_disconnect_by_func(job_, (gpointer)G_CALLBACK(onFileOpsJobPrepared), this);
  g_signal_handlers_disconnect_by_func(job_, (gpointer)G_CALLBACK(onFileOpsJobCurFile), this);
  g_signal_handlers_disconnect_by_func(job_, (gpointer)G_CALLBACK(onFileOpsJobPercent), this);
  g_signal_handlers_disconnect_by_func(job_, (gpointer)G_CALLBACK(onFileOpsJobFinished), this);
  g_signal_handlers_disconnect_by_func(job_, (gpointer)G_CALLBACK(onFileOpsJobCancelled), this);
}

FileOperation::~FileOperation() {
  if(uiTimer) {
    uiTimer->stop();
    delete uiTimer;
    uiTimer = NULL;
  }

  if(job_) {
    disconnectJob();
    g_object_unref(job_);
  }
  
  if(srcPaths)
    fm_path_list_unref(srcPaths);

  if(destPath)
    fm_path_unref(destPath);
}

bool FileOperation::run() {
  // run the job
  uiTimer = new QTimer();
  uiTimer->start(SHOW_DLG_DELAY);
  connect(uiTimer, SIGNAL(timeout()), SLOT(onUiTimeout()));

  return fm_job_run_async(FM_JOB(job_));
}

void FileOperation::onUiTimeout() {
  if(dlg) {
    dlg->setCurFile(curFile);
  }
  else{
    showDialog();
  }
}

void FileOperation::showDialog() {
  if(!dlg) {
    dlg = new FileOperationDialog(this);
    dlg->setSourceFiles(srcPaths);

    if(destPath)
      dlg->setDestPath(destPath);

    if(curFile.isEmpty()) {
      dlg->setPrepared();
      dlg->setCurFile(curFile);
    }
    uiTimer->setInterval(500); // change the interval of the timer
    // now the timer is used to update current file display
    dlg->show();
  }
}

gint FileOperation::onFileOpsJobAsk(FmFileOpsJob* job, const char* question, char*const* options, FileOperation* pThis) {
  pThis->showDialog();
  return pThis->dlg->ask(QString::fromUtf8(question), options);
}

gint FileOperation::onFileOpsJobAskRename(FmFileOpsJob* job, FmFileInfo* src, FmFileInfo* dest, char** new_name, FileOperation* pThis) {
  pThis->showDialog();
  QString newName;
  int ret = pThis->dlg->askRename(src, dest, newName);
  if(!newName.isEmpty()) {
    *new_name = g_strdup(newName.toUtf8().constData());
  }
  return ret;
}

void FileOperation::onFileOpsJobCancelled(FmFileOpsJob* job, FileOperation* pThis) {
  qDebug("file operation is cancelled!");
}

void FileOperation::onFileOpsJobCurFile(FmFileOpsJob* job, const char* cur_file, FileOperation* pThis) {
  pThis->curFile = QString::fromUtf8(cur_file);
  
  // We update the current file name in a timeout slot because drawing a string
  // in the UI is expansive. Updating the label text too often cause
  // significant impact on performance.
  // if(pThis->dlg)
  //  pThis->dlg->setCurFile(pThis->curFile);
}

FmJobErrorAction FileOperation::onFileOpsJobError(FmFileOpsJob* job, GError* err, FmJobErrorSeverity severity, FileOperation* pThis) {
  pThis->showDialog();
  return pThis->dlg->error(err, severity);
}

void FileOperation::onFileOpsJobFinished(FmFileOpsJob* job, FileOperation* pThis) {
  pThis->handleFinish();
}

void FileOperation::onFileOpsJobPercent(FmFileOpsJob* job, guint percent, FileOperation* pThis) {
  if(pThis->dlg) {
    pThis->dlg->setPercent(percent);
  }
}

void FileOperation::onFileOpsJobPrepared(FmFileOpsJob* job, FileOperation* pThis) {
  if(pThis->dlg) {
    pThis->dlg->setPrepared();
  }
}

void FileOperation::handleFinish() {
  disconnectJob();
  g_object_unref(job_);
  job_ = NULL;

  if(uiTimer) {
    uiTimer->stop();
    delete uiTimer;
    uiTimer = NULL;
  }
  
  if(dlg) {
    dlg->done(QDialog::Accepted);
    delete dlg;
    dlg = NULL;
  }
  Q_EMIT finished();
  
  if(autoDestroy_)
    delete this;
}

// static
FileOperation* FileOperation::copyFiles(FmPathList* srcFiles, FmPath* dest, QWidget* parent) {
  FileOperation* op = new FileOperation(FileOperation::Copy, srcFiles);
  op->setDestination(dest);
  op->run();
  return op;
}

// static
FileOperation* FileOperation::moveFiles(FmPathList* srcFiles, FmPath* dest, QWidget* parent) {
  FileOperation* op = new FileOperation(FileOperation::Move, srcFiles);
  op->setDestination(dest);
  op->run();
  return op;
}

//static
FileOperation* FileOperation::symlinkFiles(FmPathList* srcFiles, FmPath* dest, QWidget* parent) {
  FileOperation* op = new FileOperation(FileOperation::Link, srcFiles);
  op->setDestination(dest);
  op->run();
  return op;
}

//static
FileOperation* FileOperation::deleteFiles(FmPathList* srcFiles, bool prompt, QWidget* parent) {
  if(prompt) {
    int result = QMessageBox::warning(parent, tr("Confirm"),
                                      tr("Do you want to delete the selected files?"),
                                      QMessageBox::Yes|QMessageBox::No,
                                      QMessageBox::No);
    if(result != QMessageBox::Yes)
      return NULL;
  }

  FileOperation* op = new FileOperation(FileOperation::Delete, srcFiles);
  op->run();
  return op;
}

//static
FileOperation* FileOperation::trashFiles(FmPathList* srcFiles, bool prompt, QWidget* parent) {
  if(prompt) {
    int result = QMessageBox::warning(parent, tr("Confirm"),
                                      tr("Do you want to move the selected files to trash can?"),
                                      QMessageBox::Yes|QMessageBox::No,
                                      QMessageBox::No);
    if(result != QMessageBox::Yes)
      return NULL;
  }

  FileOperation* op = new FileOperation(FileOperation::Trash, srcFiles);
  op->run();
  return op;
}

//static
FileOperation* FileOperation::unTrashFiles(FmPathList* srcFiles, QWidget* parent) {
  FileOperation* op = new FileOperation(FileOperation::UnTrash, srcFiles);
  op->run();
  return op;
}

// static
FileOperation* FileOperation::changeAttrFiles(FmPathList* srcFiles, QWidget* parent) {
  //TODO
  FileOperation* op = new FileOperation(FileOperation::ChangeAttr, srcFiles);
  op->run();
  return op;
}
