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


#ifndef FM_MOUNTOPERATION_H
#define FM_MOUNTOPERATION_H

#include <QWidget>
#include <QDialog>
#include <QEventLoop>
#include <libfm/fm.h>
#include <gio/gio.h>

#include "ui_mount-operation-password.h"


namespace Fm {

// FIXME: the original APIs in gtk+ version of libfm for mounting devices is poor.
// Need to find a better API design which make things fully async and cancellable.

// FIXME: parent_ does not work. All dialogs shown by the mount operation has no parent window assigned.
// FIXME: Need to reconsider the propery way of API design. Blocking sync calls are handy, but
// indeed causes some problems. :-(

class MountOperation: public QObject {
Q_OBJECT

public:
  explicit MountOperation(bool interactive = true, QWidget* parent = 0);
  ~MountOperation();
  
  void mount(FmPath* path) {
    GFile* gf = fm_path_to_gfile(path);
    g_file_mount_enclosing_volume(gf, G_MOUNT_MOUNT_NONE, op, cancellable_, (GAsyncReadyCallback)onMountFileFinished, this);
    g_object_unref(gf);
  }

  void mount(GVolume* volume) {
    g_volume_mount(volume, G_MOUNT_MOUNT_NONE, op, cancellable_, (GAsyncReadyCallback)onMountVolumeFinished, this);    
  }

  void unmount(GMount* mount) {
    prepareUnmount(mount);
    g_mount_unmount_with_operation(mount, G_MOUNT_UNMOUNT_NONE, op, cancellable_, (GAsyncReadyCallback)onUnmountMountFinished, this);
  }

  void unmount(GVolume* volume) {
    GMount* mount = g_volume_get_mount(volume);
    if(!mount)
      return;
    unmount(mount);
    g_object_unref(mount);
  }
  
  void eject(GMount* mount) {
    prepareUnmount(mount);
    g_mount_eject_with_operation(mount, G_MOUNT_UNMOUNT_NONE, op, cancellable_, (GAsyncReadyCallback)onEjectMountFinished, this);
  }

  void eject(GVolume* volume) {
    GMount* mnt = g_volume_get_mount(volume);
    prepareUnmount(mnt);
    g_object_unref(mnt);
    g_volume_eject_with_operation(volume, G_MOUNT_UNMOUNT_NONE, op, cancellable_, (GAsyncReadyCallback)onEjectVolumeFinished, this);
  }

  QWidget* parent() const {
    return parent_;
  }

  void setParent(QWidget* parent) {
    parent_ = parent;
  }

  GCancellable* cancellable() const {
    return cancellable_;
  }
  
  GMountOperation* mountOperation() {
    return op;
  }
  
  void cancel() {
    g_cancellable_cancel(cancellable_);
  }

  bool isRunning() const {
    return running;
  }

  // block the operation used an internal QEventLoop and returns
  // only after the whole operation is finished.
  bool wait();

  bool autoDestroy() {
    return autoDestroy_;
  }

  void setAutoDestroy(bool destroy = true) {
    autoDestroy_ = destroy;
  }

Q_SIGNALS:
  void finished(GError* error = NULL);

private:
  void prepareUnmount(GMount* mount);

  static void onAskPassword(GMountOperation *_op, gchar* message, gchar* default_user, gchar* default_domain, GAskPasswordFlags flags, MountOperation* pThis);
  static void onAskQuestion(GMountOperation *_op, gchar* message, GStrv choices, MountOperation* pThis);
  // static void onReply(GMountOperation *_op, GMountOperationResult result, MountOperation* pThis);

  static void onAbort(GMountOperation *_op, MountOperation* pThis);
  static void onShowProcesses(GMountOperation *_op, gchar* message, GArray* processes, GStrv choices, MountOperation* pThis);
  static void onShowUnmountProgress(GMountOperation *_op, gchar* message, gint64 time_left, gint64 bytes_left, MountOperation* pThis);

  static void onMountFileFinished(GFile* file, GAsyncResult *res, MountOperation* pThis);
  static void onMountVolumeFinished(GVolume* volume, GAsyncResult *res, MountOperation* pThis);
  static void onUnmountMountFinished(GMount* mount, GAsyncResult *res, MountOperation* pThis);
  static void onEjectMountFinished(GMount* mount, GAsyncResult *res, MountOperation* pThis);
  static void onEjectVolumeFinished(GVolume* volume, GAsyncResult *res, MountOperation* pThis);

  void handleFinish(GError* error);

private:
  GMountOperation* op;
  GCancellable* cancellable_;
  QWidget* parent_;
  bool running;
  bool interactive_;
  QEventLoop* eventLoop;
  bool autoDestroy_;
};

}

#endif // FM_MOUNTOPERATION_H
