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


#ifndef FM_FILEOPERATION_H
#define FM_FILEOPERATION_H

#include "libfmqtglobals.h"
#include <QObject>
#include <libfm/fm.h>
#include "fileoperationdialog.h"

namespace Fm {

class FileOperationDialog;

class LIBFM_QT_API FileOperation : public QObject {
Q_OBJECT
public:
  enum Type {
    Copy = FM_FILE_OP_COPY,
    Move = FM_FILE_OP_MOVE,
    Link = FM_FILE_OP_LINK,
    Delete = FM_FILE_OP_DELETE,
    Trash = FM_FILE_OP_TRASH,
    Untrash = FM_FILE_OP_UNTRASH,
    ChangeAttr = FM_FILE_OP_CHANGE_ATTR
  };

public:
  explicit FileOperation(Type type, FmPathList* srcFiles, QObject* parent = 0);
  virtual ~FileOperation();

  void setDestination(FmPath* dest) {
    destPath = fm_path_ref(dest);
    fm_file_ops_job_set_dest(job_, dest);
  }

  void setChmod(mode_t newMode, mode_t newModeMask) {
    fm_file_ops_job_set_chmod(job_, newMode, newModeMask);
  }

  void setChown(gint uid, gint gid) {
    fm_file_ops_job_set_chown(job_, uid, gid);
  }

  // This only work for change attr jobs.
  void setRecursiveChattr(bool recursive) {
    fm_file_ops_job_set_recursive(job_, (gboolean)recursive);
  }

  bool run();

  void cancel() {
    if(job_)
      fm_job_cancel(FM_JOB(job_));
  }

  bool isRunning() const {
    return job_ ? fm_job_is_running(FM_JOB(job_)) : false;
  }

  bool isCancelled() const {
    return job_ ? fm_job_is_cancelled(FM_JOB(job_)) : false;
  }

  FmFileOpsJob* job() {
    return job_;
  }

  bool autoDestroy() {
    return autoDestroy_;
  }
  void setAutoDestroy(bool destroy = true) {
    autoDestroy_ = destroy;
  }

  Type type() {
    return (Type)job_->type;
  }
  
  // convinient static functions
  static FileOperation* copyFiles(FmPathList* srcFiles, FmPath* dest, QWidget* parent = 0);
  static FileOperation* moveFiles(FmPathList* srcFiles, FmPath* dest, QWidget* parent = 0);
  static FileOperation* symlinkFiles(FmPathList* srcFiles, FmPath* dest, QWidget* parent = 0);
  static FileOperation* deleteFiles(FmPathList* srcFiles, bool promp = true, QWidget* parent = 0);
  static FileOperation* trashFiles(FmPathList* srcFiles, bool promp = true, QWidget* parent = 0);
  static FileOperation* changeAttrFiles(FmPathList* srcFiles, QWidget* parent = 0);

Q_SIGNALS:
  void finished();
  
private:
  static gint onFileOpsJobAsk(FmFileOpsJob* job, const char* question, char* const* options, FileOperation* pThis);
  static gint onFileOpsJobAskRename(FmFileOpsJob* job, FmFileInfo* src, FmFileInfo* dest, char** new_name, FileOperation* pThis);
  static FmJobErrorAction onFileOpsJobError(FmFileOpsJob* job, GError* err, FmJobErrorSeverity severity, FileOperation* pThis);
  static void onFileOpsJobPrepared(FmFileOpsJob* job, FileOperation* pThis);
  static void onFileOpsJobCurFile(FmFileOpsJob* job, const char* cur_file, FileOperation* pThis);
  static void onFileOpsJobPercent(FmFileOpsJob* job, guint percent, FileOperation* pThis);
  static void onFileOpsJobFinished(FmFileOpsJob* job, FileOperation* pThis);
  static void onFileOpsJobCancelled(FmFileOpsJob* job, FileOperation* pThis);

  void handleFinish();
  void disconnectJob();
  void showDialog();

private Q_SLOTS:
  void onUiTimeout();
  
private:
  FmFileOpsJob* job_;
  FileOperationDialog* dlg;
  FmPath* destPath;
  FmPathList* srcPaths;
  QTimer* uiTimer;
  QString curFile;
  bool autoDestroy_;

};

}

#endif // FM_FILEOPERATION_H
