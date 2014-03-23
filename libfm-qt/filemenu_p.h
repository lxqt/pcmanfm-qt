/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef FM_FILEMENU_P_H
#define FM_FILEMENU_P_H

#include "icontheme.h"
#include "fileoperation.h"
#ifdef CUSTOM_ACTIONS
#include <libfm/fm-actions.h>
#endif
#include <QDebug>

namespace Fm {

class AppInfoAction : public QAction {
  Q_OBJECT
public:
  explicit AppInfoAction(GAppInfo* app, QObject* parent = 0):
    QAction(QString::fromUtf8(g_app_info_get_name(app)), parent),
    appInfo_(G_APP_INFO(g_object_ref(app))) {
    setToolTip(QString::fromUtf8(g_app_info_get_description(app)));
    GIcon* gicon = g_app_info_get_icon(app);
    QIcon icon = IconTheme::icon(gicon);
    setIcon(icon);
  }

  virtual ~AppInfoAction() {
    if(appInfo_)
      g_object_unref(appInfo_);
  }

  GAppInfo* appInfo() const {
    return appInfo_;
  }

private:
  GAppInfo* appInfo_;
};

#ifdef CUSTOM_ACTIONS
class CustomAction : public QAction {
  Q_OBJECT
public:
  explicit CustomAction(FmFileActionItem* item, QObject* parent = NULL):
    QAction(QString::fromUtf8(fm_file_action_item_get_name(item)), parent),
    item_(reinterpret_cast<FmFileActionItem*>(fm_file_action_item_ref(item))) {
    const char* icon_name = fm_file_action_item_get_icon(item);
    if(icon_name)
      setIcon(QIcon::fromTheme(icon_name));
  }

  virtual ~CustomAction() {
    fm_file_action_item_unref(item_);
  }

  FmFileActionItem* item() {
    return item_;
  }

private:
  FmFileActionItem* item_;
};

#endif

} // namespace Fm

#endif
