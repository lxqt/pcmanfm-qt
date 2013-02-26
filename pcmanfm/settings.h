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

  // setter/getter functions
  QString profileName() const {
    return profileName_;
  }

  QString iconThemeName() const {
    return iconThemeName_;
  }
  
  void setIconThemeName(QString iconThemeName) {
    iconThemeName_ = iconThemeName;
  }

  int bookmarkOpenMethod() {
    return bookmarkOpenMethod_;
  }

  void setBookmarkOpenMethod(int bookmarkOpenMethod) {
    bookmarkOpenMethod_ = bookmarkOpenMethod;
  }

  QString suCommand() const {
    return suCommand_;
  }
  
  void setSuCommand(QString suCommand) {
    suCommand_ = suCommand;
  }

  bool mountOnStartup() const {
    return mountOnStartup_;
  }

  void setMountOnStartup(bool mountOnStartup) {
    mountOnStartup_ = mountOnStartup;
  }

  bool mountRemovable() {
    return mountRemovable_;
  }

  void setMountRemovable(bool mountRemovable) {
    mountRemovable_ = mountRemovable;
  }

  bool autoRun() const {
    return autoRun_;
  }

  void setAutoRun(bool autoRun) {
    autoRun_ = autoRun;
  }


  int wallpaperMode() const {
    return wallpaperMode_;
  }

  void setWallpaperMode(int wallpaperMode) {
    wallpaperMode_ = wallpaperMode;
  }

  QString wallpaper() const {
    return wallpaper_;
  }

  void setWallpaper(QString wallpaper) {
    wallpaper_ = wallpaper;
  }

  const QColor& desktopBgColor() const {
    return desktopBgColor_;
  }

  void setDesktopBgColor(QColor desktopBgColor) {
    desktopBgColor_ = desktopBgColor;
  }

  const QColor& desktopFgColor() const {
    return desktopFgColor_;
  }

  void setDesktopFgColor(QColor desktopFgColor) {
    desktopFgColor_ = desktopFgColor;
  }

  const QColor& desktopShadowColor() const {
    return desktopShadowColor_;
  }

  void setDesktopShadowColor(QColor desktopShadowColor) {
    desktopShadowColor_ = desktopShadowColor;
  }

  // desktop_font=Sans 12
  // bool showWmMenu;
  bool desktopShowHidden() const {
    return desktopShowHidden_;
  }

  void setDesktopShowHidden(bool desktopShowHidden) {
    desktopShowHidden_ = desktopShowHidden;
  }

  Qt::SortOrder desktopSortOrder() const {
    return desktopSortOrder_;
  }

  void setDesktopSortOrder(Qt::SortOrder desktopSortOrder) {
    desktopSortOrder_ = desktopSortOrder;
  }

  Fm::FolderModel::ColumnId desktopSortColumn() const {
    return desktopSortColumn_;
  }

  void setDesktopSortColumn(Fm::FolderModel::ColumnId desktopSortColumn) {
    desktopSortColumn_ = desktopSortColumn;
  }

  bool alwaysShowTabs() const {
    return alwaysShowTabs_;
  }

  void setAlwaysShowTabs(bool alwaysShowTabs) {
    alwaysShowTabs_ = alwaysShowTabs;
  }

  int windowWidth() const {
    return windowWidth_;
  }

  void setWindowWidth(int windowWidth) {
    windowWidth_ = windowWidth;
  }

  int windowHeight() const {
    return windowHeight_;
  }

  void setWindowHeight(int windowHeight) {
    windowHeight_ = windowHeight;
  }

  int splitterPos() const {
    return splitterPos_;
  }

  void setSplitterPos(int splitterPos) {
    splitterPos_ = splitterPos;
  }

  int sidePaneMode() const {
    return sidePaneMode_;
  }

  void setSidePaneMode(int sidePaneMode) {
    sidePaneMode_ = sidePaneMode;
  }

  Fm::FolderView::ViewMode viewMode() const {
    return viewMode_;
  }

  void setViewMode(Fm::FolderView::ViewMode viewMode) {
    viewMode_ = viewMode;
  }

  bool showHidden() const {
    return showHidden_;
  }

  void setShowHidden(bool showHidden) {
    showHidden_ = showHidden;
  }

  Qt::SortOrder sortOrder() const {
    return sortOrder_;
  }

  void setSortOrder(Qt::SortOrder sortOrder) {
    sortOrder_ = sortOrder;
  }

  Fm::FolderModel::ColumnId sortColumn() const {
    return sortColumn_;
  }

  void setSortColumn(Fm::FolderModel::ColumnId sortColumn) {
    sortColumn_ = sortColumn;
  }

  // settings for use with libfm
  bool singleClick() const {
    return singleClick_;
  }

  void setSingleClick(bool singleClick) {
    singleClick_ = singleClick;
  }

  bool useTrash() const {
    return useTrash_;
  }

  void setUseTrash(bool useTrash) {
    useTrash_ = useTrash;
  }

  bool confirmDelete() const {
    return confirmDelete_;
  }

  void setConfirmDelete(bool confirmDelete) {
    confirmDelete_ = confirmDelete;
  }

  // bool thumbnailLocal_;
  // bool thumbnailMax;

  int bigIconSize() const {
    return bigIconSize_;
  }

  void setBigIconSize(int bigIconSize) {
    bigIconSize_ = bigIconSize;
  }

  int smallIconSize() const {
    return smallIconSize_;
  }

  void setSmallIconSize(int smallIconSize) {
    smallIconSize_ = smallIconSize;
  }

  int sidePaneIconSize() const {
    return sidePaneIconSize_;
  }

  void setSidePaneIconSize(int sidePaneIconSize) {
    sidePaneIconSize_ = sidePaneIconSize;
  }

  int thumbnailIconSize() const {
    return thumbnailIconSize_;
  }

  void setThumbnailIconSize(int thumbnailIconSize) {
    thumbnailIconSize_ = thumbnailIconSize;
  }
  
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
