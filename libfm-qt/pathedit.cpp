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


#include "pathedit.h"
#include <QCompleter>
#include <QStringListModel>
#include <QStringBuilder>
#include <libfm/fm.h>

using namespace Fm;

PathEdit::PathEdit(QWidget* parent):
  QLineEdit(parent),
  cancellable_(NULL),
  model_(new QStringListModel()),
  completer_(new QCompleter()) {
  setCompleter(completer_);
  completer_->setModel(model_);
  connect(this, SIGNAL(textChanged(QString)), SLOT(onTextChanged(QString)));
}

PathEdit::~PathEdit() {
  delete completer_;
  if(model_)
    delete model_;
  if(cancellable_) {
    g_cancellable_cancel(cancellable_);
    g_object_unref(cancellable_);
  }
}

void PathEdit::focusInEvent(QFocusEvent* e) {
  QLineEdit::focusInEvent(e);
}

void PathEdit::focusOutEvent(QFocusEvent* e) {
  QLineEdit::focusOutEvent(e);
}

void PathEdit::onTextChanged(const QString& text) {
  int pos = text.lastIndexOf('/');
  if(pos >= 0)
    ++pos;
  else
    pos = text.length();
  QString newPrefix = text.left(pos);
  if(currentPrefix_ != newPrefix) {
    currentPrefix_ = newPrefix;
    reloadCompleter();
  }
}

struct JobData {
  GCancellable* cancellable;
  GFile* dirName;
  QStringList subDirs;
  PathEdit* edit;
  
  ~JobData() {
    g_object_unref(dirName);
    g_object_unref(cancellable);
  }

  static void freeMe(JobData* data) {
    delete data;
  }
};

void PathEdit::reloadCompleter() {
  // parent dir has been changed, reload dir list
  // if(currentPrefix_[0] == "~") { // special case for home dir
  // cancel running dir-listing jobs, if there's any
  if(cancellable_) {
    g_cancellable_cancel(cancellable_);
    g_object_unref(cancellable_);
  }
  // launch a new job to do dir listing
  JobData* data = new JobData();
  data->edit = this;
  // need to use fm_file_new_for_commandline_arg() rather than g_file_new_for_commandline_arg().
  // otherwise, our own vfs, such as menu://, won't be loaded.
  data->dirName = fm_file_new_for_commandline_arg(currentPrefix_.toLocal8Bit().constData());
  qDebug("load: %s", g_file_get_uri(data->dirName));
  cancellable_ = g_cancellable_new();
  data->cancellable = (GCancellable*)g_object_ref(cancellable_);
  g_io_scheduler_push_job((GIOSchedulerJobFunc)jobFunc,
                          data, (GDestroyNotify)JobData::freeMe,
                          G_PRIORITY_LOW, cancellable_);
}

gboolean PathEdit::jobFunc(GIOSchedulerJob* job, GCancellable* cancellable, gpointer user_data) {
  JobData* data = reinterpret_cast<JobData*>(user_data);
  GError *err = NULL;
  GFileEnumerator* enu = g_file_enumerate_children(data->dirName,
                                                   // G_FILE_ATTRIBUTE_STANDARD_NAME","
                                                   G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME","
                                                   G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                                   G_FILE_QUERY_INFO_NONE, cancellable,
                                                   &err);
  if(enu) {
    while(!g_cancellable_is_cancelled(cancellable)) {
      GFileInfo* inf = g_file_enumerator_next_file(enu, cancellable, &err);
      if(inf) {
        GFileType type = g_file_info_get_file_type(inf);
        if(type == G_FILE_TYPE_DIRECTORY) {
          const char* name = g_file_info_get_display_name(inf);
          // FIXME: encoding conversion here?
          data->subDirs.append(QString::fromUtf8(name));
        }
        g_object_unref(inf);
      }
      else {
        if(err) {
          g_error_free(err);
          err = NULL;
        }
        else /* EOF */
          break;
      }
    }
    g_file_enumerator_close(enu, cancellable, NULL);
    g_object_unref(enu);
  }
  // finished! let's update the UI in the main thread
  g_io_scheduler_job_send_to_mainloop(job, onJobFinished, data, NULL);
  return FALSE;
}

// This callback function is called from main thread so it's safe to access the GUI
gboolean PathEdit::onJobFinished(gpointer user_data) {
  JobData* data = reinterpret_cast<JobData*>(user_data);
  PathEdit* pThis = data->edit;
  if(!g_cancellable_is_cancelled(data->cancellable)) {
    // update the completer only if the job is not cancelled
    QStringList::iterator it;
    for(it = data->subDirs.begin(); it != data->subDirs.end(); ++it) {
      qDebug("%s", it->toUtf8().constData());
      *it = (pThis->currentPrefix_ % *it);
    }
    pThis->model_->setStringList(data->subDirs);
    // trigger completion manually
    if(pThis->hasFocus())
      pThis->completer_->complete();
  }
  else
    pThis->model_->setStringList(QStringList());
  g_object_unref(pThis->cancellable_);
  pThis->cancellable_ = NULL;
  return TRUE;
}
