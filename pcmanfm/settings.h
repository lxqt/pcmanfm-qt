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


#ifndef PCMANFM_SETTINGS_H
#define PCMANFM_SETTINGS_H

#include <QObject>
#include <libfm/fm.h>
#include "folderview.h"
#include "foldermodel.h"
#include "desktopwindow.h"
#include "thumbnailloader.h"

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

  QString profileDir(QString profile, bool useFallback = false);

  // setter/getter functions
  QString profileName() const {
    return profileName_;
  }

  QString fallbackIconThemeName() const {
    return fallbackIconThemeName_;
  }
  
  bool useFallbackIconTheme() const {
    return useFallbackIconTheme_;
  }
  
  void setFallbackIconThemeName(QString iconThemeName) {
    fallbackIconThemeName_ = iconThemeName;
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

  QString terminalDirCommand() {
    return terminalDirCommand_;
  }
  void setTerminalDirCommand(QString terminalCommand) {
    terminalDirCommand_ = terminalCommand;
  }

  QString terminalExecCommand() {
    return terminalExecCommand_;
  }
  void setTerminalExecCommand(QString terminalCommand) {
    terminalExecCommand_ = terminalCommand;
    // override the settings in libfm FmConfig.
    g_free(fm_config->terminal);
    fm_config->terminal = g_strdup(terminalExecCommand_.toLocal8Bit().constData());
  }

  QString archiver() const {
    return archiver_;
  }

  void setArchiver(QString archiver) {
    archiver_ = archiver;
    // override libfm FmConfig
    g_free(fm_config->archiver);
    fm_config->archiver = g_strdup(archiver_.toLocal8Bit().constData());
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


  DesktopWindow::WallpaperMode wallpaperMode() const {
    return DesktopWindow::WallpaperMode(wallpaperMode_);
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

  QFont desktopFont() const {
    return desktopFont_;
  }
  
  void setDesktopFont(QFont font) {
    desktopFont_ = font;
  }

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

  bool showTabClose() const {
    return showTabClose_;
  }
  
  void setShowTabClose(bool showTabClose) {
    showTabClose_ = showTabClose;
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

  bool showThumbnails() {
    return showThumbnails_;
  }
  
  void setShowThumbnails(bool show) {
    showThumbnails_ = show;
  }
  
  void setThumbnailLocalFilesOnly(bool value) {
    Fm::ThumbnailLoader::setLocalFilesOnly(value);
  }

  bool thumbnailLocalFilesOnly() {
    return Fm::ThumbnailLoader::localFilesOnly();
  }

  int maxThumbnailFileSize() {
    return Fm::ThumbnailLoader::maxThumbnailFileSize();
  }

  void setMaxThumbnailFileSize(int size) {
    Fm::ThumbnailLoader::setMaxThumbnailFileSize(size);
  }

  void setThumbnailIconSize(int thumbnailIconSize) {
    thumbnailIconSize_ = thumbnailIconSize;
  }

  bool siUnit() {
    return siUnit_;
  }

  void setSiUnit(bool siUnit) {
    siUnit_ = siUnit;
    // override libfm FmConfig settings. FIXME: should we do this?
    fm_config->si_unit = (gboolean)siUnit_;
  }

 
private:
  QString profileName_;

  // PCManFM specific
  QString fallbackIconThemeName_;
  bool useFallbackIconTheme_;

  int bookmarkOpenMethod_;
  QString suCommand_;
  QString terminalDirCommand_;
  QString terminalExecCommand_;
  bool mountOnStartup_;
  bool mountRemovable_;
  bool autoRun_;

  int wallpaperMode_;
  QString wallpaper_;
  QColor desktopBgColor_;
  QColor desktopFgColor_;
  QColor desktopShadowColor_;
  QFont desktopFont_;

  bool desktopShowHidden_;
  Qt::SortOrder desktopSortOrder_;
  Fm::FolderModel::ColumnId desktopSortColumn_;

  bool alwaysShowTabs_;
  bool showTabClose_;
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

  bool showThumbnails_;

  QString archiver_;
  bool siUnit_;

  int bigIconSize_;
  int smallIconSize_;
  int sidePaneIconSize_;
  int thumbnailIconSize_;
};

}

#endif // PCMANFM_SETTINGS_H
