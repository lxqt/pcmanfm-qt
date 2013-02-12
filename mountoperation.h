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
#include <libfm/fm.h>
#include <gio/gio.h>

namespace Fm {

// FIXME: the original APIs in gtk+ version of libfm for mounting devices is poor.
// Need to find a better API design which make things fully async and cancellable.
  
class MountOperation
{
public:
  explicit MountOperation(QWidget* parent = 0);
  ~MountOperation();

  bool mount(FmPath* path, bool interactive = true);
  bool mount(GVolume* vol, bool interactive = true);
  bool unmount(GMount* mount, bool interactive = true);
  bool unmount(GVolume* vol, bool interactive = true);
  bool eject(GMount* mount, bool interactive = true);
  bool eject(GVolume* vol, bool interactive = true);

  QWidget* parent() {
    return parent_;
  }
  void setParent(QWidget* parent) {
    parent_ = parent;
  }

private:
  void prepareUnmount(GMount* mount);
  bool doMount(GObject* obj, int action, gboolean interactive);

  static void onAskPassword(GMountOperation *_op, gchar* message, gchar* default_user, gchar* default_domain, GAskPasswordFlags flags, MountOperation* pThis);
  static void onAskQuestion(GMountOperation *_op, gchar* message, GStrv choices, MountOperation* pThis);
  static void onReply(GMountOperation *_op, GMountOperationResult result, MountOperation* pThis);

  static void onAbort(GMountOperation *_op, MountOperation* pThis);
  static void onShowProcesses(GMountOperation *_op, gchar* message, GArray* processes, GStrv choices, MountOperation* pThis);
  static void onShowUnmountProgress(GMountOperation *_op, gchar* message, gint64 time_left, gint64 bytes_left, MountOperation* pThis);

private:
  GMountOperation* op;
  QWidget* parent_;
};

}

#endif // FM_MOUNTOPERATION_H
