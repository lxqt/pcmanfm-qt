/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  PCMan <email>

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


#ifndef PCMANFM_SETTINGS_H
#define PCMANFM_SETTINGS_H

#include <QObject>
#include <libfm/fm.h>
#include "folderview.h"
#include "foldermodel.h"

namespace PCManFM {

class Settings : public QObject {
Q_OBJECT
public:
  Settings();
  virtual ~Settings();

  bool load(QString profile = "default");
  bool save(QString profile = QString());

  bool loadFile(QString filePath);
  bool saveFile(QString filePath);

  QString profileDir(QString profile = QString());

private:
  QString profileName_;

  // PCManFM specific
  QString iconThemeName_;

  int bookmarkOpenMethod_;
  QString suCommand_;
  bool mountOnStartup_;
  bool mountRemovable_;
  bool autoRun_;

  int wallpaperMode_;
  QString wallpaper_;
  QColor desktopBgColor_;
  QColor desktopFgColor_;
  QColor desktopShadowColor_;
  // desktop_font=Sans 12
  // bool showWmMenu;
  bool desktopShowHidden_;
  Qt::SortOrder desktopSortOrder_;
  Fm::FolderModel::ColumnId desktopSortColumn_;

  bool alwaysShowTabs_;
  int windowWidth_;
  int windowHeight_;
  int splitterPos_;
  int sidePaneMode_;

  Fm::FolderView::ViewMode viewMode_;
  bool showHidden_;
  Qt::SortOrder sortOrder_;
  Fm::FolderModel::ColumnId sortColumn_;

  // settings for use with libfm
  bool singleClick_;
  bool useTrash_;
  bool confirmDelete_;
  // bool thumbnailLocal_;
  // bool thumbnailMax;

  int bigIconSize_;
  int smallIconSize_;
  int sidePaneIconSize_;
  int thumbnailIconSize_;
};

}

#endif // PCMANFM_SETTINGS_H
