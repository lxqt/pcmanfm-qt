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


#include "mountoperation.h"
#include <glib/gi18n.h> // for _()
#include <QMessageBox>

#include "ui_mount-operation-password.h"

using namespace Fm;

class MountOpAskPasswordDialog : public QDialog {
public:
  explicit MountOpAskPasswordDialog(QWidget* parent = 0, Qt::WindowFlags f = 0) : QDialog(parent) {
    ui.setupUi(this);
  }

  virtual ~MountOpAskPasswordDialog() {
  }

private:
  Ui::MountOpAskPasswordDialog ui;
};


MountOperation::MountOperation(QWidget* parent) {
  op = g_mount_operation_new();
  g_signal_connect(op, "ask-password", G_CALLBACK(onAskPassword), this);
  g_signal_connect(op, "ask-question", G_CALLBACK(onAskQuestion), this);
  g_signal_connect(op, "reply", G_CALLBACK(onReply), this);

#if GLIB_CHECK_VERSION(2, 20, 0)
//  g_signal_connect(op, "aborted", G_CALLBACK(), this);
#endif
#if GLIB_CHECK_VERSION(2, 22, 0)
//  g_signal_connect(op, "show-processes", G_CALLBACK(), this);
#endif
#if GLIB_CHECK_VERSION(2, 34, 0)
//  g_signal_connect(op, "show-unmount-progress", G_CALLBACK(), this);
#endif
}

MountOperation::~MountOperation() {
  if(op)
    g_object_unref(op);
}

void MountOperation::onAbort(GMountOperation* _op, MountOperation* pThis) {

}

void MountOperation::onAskPassword(GMountOperation* _op, gchar* message, gchar* default_user, gchar* default_domain, GAskPasswordFlags flags, MountOperation* pThis) {
  qDebug("ask password");
  MountOpAskPasswordDialog* dlg = new MountOpAskPasswordDialog(pThis->parent());
  int res = dlg->exec();
  if(res == QDialog::Accepted) {
    g_mount_operation_reply(_op, G_MOUNT_OPERATION_HANDLED);
  }
  else {
    g_mount_operation_reply(_op, G_MOUNT_OPERATION_ABORTED);
  }
  dlg->done(res);
}

void MountOperation::onAskQuestion(GMountOperation* _op, gchar* message, GStrv choices, MountOperation* pThis) {
  qDebug("ask question");
}

void MountOperation::onReply(GMountOperation* _op, GMountOperationResult result, MountOperation* pThis) {
  qDebug("reply");
}

void MountOperation::onShowProcesses(GMountOperation* _op, gchar* message, GArray* processes, GStrv choices, MountOperation* pThis) {
  qDebug("show processes");
}

void MountOperation::onShowUnmountProgress(GMountOperation* _op, gchar* message, gint64 time_left, gint64 bytes_left, MountOperation* pThis) {
  qDebug("show unmount progress");
}

typedef enum {
    MOUNT_VOLUME,
    MOUNT_GFILE,
    UMOUNT_MOUNT,
    EJECT_MOUNT,
    EJECT_VOLUME
}MountAction;

struct MountData {
    GMainLoop *loop;
    MountAction action;
    GError* err;
    gboolean ret;
};

static void onMountActionFinished(GObject* src, GAsyncResult *res, gpointer user_data) {
  struct MountData* data = user_data;
  switch(data->action)
  {
  case MOUNT_VOLUME:
      data->ret = g_volume_mount_finish(G_VOLUME(src), res, &data->err);
      break;
  case MOUNT_GFILE:
      data->ret = g_file_mount_enclosing_volume_finish(G_FILE(src), res, &data->err);
      break;
  case UMOUNT_MOUNT:
      data->ret = g_mount_unmount_with_operation_finish(G_MOUNT(src), res, &data->err);
      break;
  case EJECT_MOUNT:
      data->ret = g_mount_eject_with_operation_finish(G_MOUNT(src), res, &data->err);
      break;
  case EJECT_VOLUME:
      data->ret = g_volume_eject_with_operation_finish(G_VOLUME(src), res, &data->err);
      break;
  }
  g_main_loop_quit(data->loop);
}

void MountOperation::prepareUnmount(GMount* mount) {
  /* ensure that CWD is not on the mounted filesystem. */
  char* cwd_str = g_get_current_dir();
  GFile* cwd = g_file_new_for_path(cwd_str);
  GFile* root = g_mount_get_root(mount);
  g_free(cwd_str);
  /* FIXME: This cannot cover 100% cases since symlinks are not checked.
    * There may be other cases that cwd is actually under mount root
    * but checking prefix is not enough. We already did our best, though. */
  if(g_file_has_prefix(cwd, root))
      g_chdir("/");
  g_object_unref(cwd);
  g_object_unref(root);
}


bool MountOperation::doMount(GObject* obj, int action, gboolean interactive) {
  gboolean ret;
  struct MountData* data = g_new0(struct MountData, 1);
  // GMountOperation* op = interactive ? gtk_mount_operation_new(parent) : NULL;
  GCancellable* cancellable = g_cancellable_new();

  data->loop = g_main_loop_new (NULL, TRUE);
  data->action = action;

  switch(data->action) {
  case MOUNT_VOLUME:
      g_volume_mount(G_VOLUME(obj), 0, op, cancellable, onMountActionFinished, data);
      break;
  case MOUNT_GFILE:
      g_file_mount_enclosing_volume(G_FILE(obj), 0, op, cancellable, onMountActionFinished, data);
      break;
  case UMOUNT_MOUNT:
      prepareUnmount(G_MOUNT(obj));
      g_mount_unmount_with_operation(G_MOUNT(obj), G_MOUNT_UNMOUNT_NONE, op, cancellable, onMountActionFinished, data);
      break;
  case EJECT_MOUNT:
      prepareUnmount(G_MOUNT(obj));
      g_mount_eject_with_operation(G_MOUNT(obj), G_MOUNT_UNMOUNT_NONE, op, cancellable, onMountActionFinished, data);
      break;
  case EJECT_VOLUME: {
	  GMount* mnt = g_volume_get_mount(G_VOLUME(obj));
	  prepareUnmount(mnt);
	  g_object_unref(mnt);
	  g_volume_eject_with_operation(G_VOLUME(obj), G_MOUNT_UNMOUNT_NONE, op, cancellable, onMountActionFinished, data);
      }
      break;
  }

  if (g_main_loop_is_running(data->loop))
      g_main_loop_run(data->loop);

  g_main_loop_unref(data->loop);

  ret = data->ret;
  if(data->err) {
    if(interactive) {
      if(data->err->domain == G_IO_ERROR) {
	if(data->err->code == G_IO_ERROR_FAILED) {
	  /* Generate a more human-readable error message instead of using a gvfs one. */
	  /* The original error message is something like:
	    * Error unmounting: umount exited with exit code 1:
	    * helper failed with: umount: only root can unmount
	    * UUID=18cbf00c-e65f-445a-bccc-11964bdea05d from /media/sda4 */
	  /* Why they pass this back to us?
	    * This is not human-readable for the users at all. */
	  if(strstr(data->err->message, "only root can ")) {
	      g_free(data->err->message);
	      data->err->message = g_strdup(_("Only system administrators have the permission to do this."));
	  }
	}
	else if(data->err->code == G_IO_ERROR_FAILED_HANDLED)
	    interactive = FALSE;
      }
      if(interactive)
	QMessageBox::critical(parent_, QObject::tr("Error"), QString::fromUtf8(data->err->message));
    }
    g_error_free(data->err);
  }
  g_free(data);
  g_object_unref(cancellable);
  if(op)
      g_object_unref(op);
  return ret;
}

bool MountOperation::mount(FmPath* path, bool interactive) {
  GFile* gf = fm_path_to_gfile(path);
  gboolean ret = doMount(G_OBJECT(gf), MOUNT_GFILE, interactive);
  g_object_unref(gf);
  return ret;
}

bool MountOperation::mount(GVolume* vol, bool interactive) {
  return doMount(G_OBJECT(vol), MOUNT_VOLUME, interactive);
}

bool MountOperation::unmount(GMount* mount, bool interactive) {
  return doMount(G_OBJECT(mount), UMOUNT_MOUNT, interactive);
}

bool MountOperation::unmount(GVolume* vol, bool interactive) {
  GMount* mount = g_volume_get_mount(vol);
  gboolean ret;
  if(!mount)
      return FALSE;
  ret = doMount(G_OBJECT(vol), UMOUNT_MOUNT, interactive);
  g_object_unref(mount);
  return ret;
}

bool MountOperation::eject(GMount* mount, bool interactive) {
  return doMount(G_OBJECT(mount), EJECT_MOUNT, interactive);
}

bool MountOperation::eject(GVolume* vol, bool interactive) {
  return doMount(G_OBJECT(vol), EJECT_VOLUME, interactive);
}
