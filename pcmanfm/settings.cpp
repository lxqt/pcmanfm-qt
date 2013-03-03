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


#include "settings.h"
#include <QDir>
#include <QStringBuilder>
#include <QSettings>
// #include <QDesktopServices>

using namespace PCManFM;

Settings::Settings():
  QObject(),
  iconThemeName_(),
  bookmarkOpenMethod_(0),
  suCommand_(),
  terminalCommand_(),
  mountOnStartup_(true),
  mountRemovable_(true),
  autoRun_(true),
  wallpaperMode_(0),
  wallpaper_(),
  desktopBgColor_(),
  desktopFgColor_(),
  desktopShadowColor_(),
  // desktop_font=Sans 12
  // bool showWmMenu;
  desktopShowHidden_(false),
  desktopSortOrder_(Qt::AscendingOrder),
  desktopSortColumn_(Fm::FolderModel::ColumnFileName),
  alwaysShowTabs_(true),
  showTabClose_(true),
  windowWidth_(640),
  windowHeight_(480),
  splitterPos_(120),
  sidePaneMode_(0),
  viewMode_(Fm::FolderView::IconMode),
  showHidden_(false),
  sortOrder_(Qt::AscendingOrder),
  sortColumn_(Fm::FolderModel::ColumnFileName),
  // settings for use with libfm
  singleClick_(false),
  useTrash_(true),
  confirmDelete_(true),
  // bool thumbnailLocal_;
  // bool thumbnailMax;
  archiver_(),
  siUnit_(false),
  bigIconSize_(48),
  smallIconSize_(24),
  sidePaneIconSize_(24),
  thumbnailIconSize_(128) {
}

Settings::~Settings() {

}

QString Settings::profileDir(QString profile) {
  // NOTE: it's a shame that QDesktopServices does not handle XDG_CONFIG_HOME
  QString dirName = QLatin1String(qgetenv("XDG_CONFIG_HOME"));
  if (dirName.isEmpty())
    dirName = QDir::homePath() % QLatin1String("/.config");
  dirName = dirName % "/pcmanfm-qt/" % profile;
  return dirName;
}

bool Settings::load(QString profile) {
  QString fileName = profileDir(profile) % "/settings.conf";
  profileName_ = profile;
  return loadFile(fileName);
}

bool Settings::save(QString profile) {
  QString fileName = profileDir(profile.isEmpty() ? profileName_ : profile) % "/settings.conf";
  return saveFile(fileName);
}

bool Settings::loadFile(QString filePath) {
  QSettings settings(filePath, QSettings::IniFormat);

  settings.beginGroup("System");
  iconThemeName_ = settings.value("IconThemeName").toString();
  if(iconThemeName_.isEmpty()) {
    // FIXME: we should choose one from installed icon themes or get
    // the value from XSETTINGS instead of hard code a fallback value.
    iconThemeName_ = "elementary"; // fallback icon theme name
  }
  suCommand_ = settings.value("SuCommand").toString();
  setTerminalCommand(settings.value("TerminalCommand").toString());
  setArchiver(settings.value("Archiver").toString());
  setSiUnit(settings.value("SIUnit", false).toBool());
  settings.endGroup();

  settings.beginGroup("Behavior");
  bookmarkOpenMethod_ = settings.value("BookmarkOpenMethod").toInt();
  // settings for use with libfm
  useTrash_ = settings.value("UseTrash", true).toBool();
  singleClick_ = settings.value("SingleClick", false).toBool();
  confirmDelete_ = settings.value("ConfirmDelete", true).toBool();
  // bool thumbnailLocal_;
  // bool thumbnailMax;
  settings.endGroup();

  settings.beginGroup("Desktop");
  wallpaperMode_ = settings.value("WallpaperMode").toInt();
  wallpaper_ = settings.value("Wallpaper").toString();
  desktopBgColor_.setNamedColor(settings.value("BgColor", "#000000").toString());
  desktopFgColor_.setNamedColor(settings.value("FgColor", "#ffffff").toString());
  desktopShadowColor_.setNamedColor(settings.value("ShadowColor", "#000000").toString());
  // desktop_font=Sans 12
  // bool showWmMenu;
  desktopShowHidden_ = settings.value("ShowHidden", false).toBool();
  // FIXME: we need to convert these values to strings
  desktopSortOrder_ = settings.value("SortOrder", Qt::AscendingOrder).toInt();
  desktopSortColumn_ = settings.value("SortColumn", Fm::FolderModel::ColumnFileName).toInt();
  settings.endGroup();

  settings.beginGroup("Volume");
  mountOnStartup_ = settings.value("MountOnStartup", true).toBool();
  mountRemovable_ = settings.value("MountRemovable", true).toBool();
  autoRun_ = settings.value("AutoRun", true).toBool();
  settings.endGroup();

  settings.beginGroup("FolderView");
  viewMode_ = settings.value("Mode", Fm::FolderView::IconMode).toInt();
  showHidden_ = settings.value("ShowHidden", false).toBool();
  sortOrder_ = settings.value("SortOrder", Qt::AscendingOrder).toInt();
  sortColumn_ = settings.value("SortColumn", Fm::FolderModel::ColumnFileName).toInt();

  // override config in libfm's FmConfig
  bigIconSize_ = settings.value("BigIconSize", 48).toInt();
  smallIconSize_ = settings.value("SmallIconSize_", 24).toInt();
  sidePaneIconSize_ = settings.value("SidePaneIconSize_", 24).toInt();
  thumbnailIconSize_ = settings.value("ThumbnailIconSize_", 128).toInt();
  settings.endGroup();
  
  settings.beginGroup("Window");
  windowWidth_ = settings.value("Width", 640).toInt();
  windowHeight_ = settings.value("Height", 680).toInt();
  alwaysShowTabs_ = settings.value("AlwaysShowTabs", true).toBool();
  showTabClose_ = settings.value("ShowTabClose", true).toBool();
  splitterPos_ = settings.value("SplitterPos", 150).toInt();
  sidePaneMode_ = settings.value("SidePaneMode").toInt();
  settings.endGroup();

}

bool Settings::saveFile(QString filePath) {
  QSettings settings(filePath, QSettings::IniFormat);

  settings.beginGroup("System");
  settings.setValue("IconThemeName", iconThemeName_);
  settings.setValue("SuCommand", suCommand_);
  settings.setValue("TerminalCommand", terminalCommand_);
  settings.setValue("Archiver", archiver_);
  settings.setValue("SIUnit", siUnit_);
  settings.endGroup();

  settings.beginGroup("Behavior");
  settings.setValue("BookmarkOpenMethod", bookmarkOpenMethod_);
  // settings for use with libfm
  settings.setValue("UseTrash", useTrash_);
  settings.setValue("SingleClick", singleClick_);
  settings.setValue("ConfirmDelete", confirmDelete_);
  // bool thumbnailLocal_;
  // bool thumbnailMax;
  settings.endGroup();

  settings.beginGroup("Desktop");
  settings.setValue("WallpaperMode", wallpaperMode_);
  settings.setValue("Wallpaper", wallpaper_);
  settings.setValue("BgColor", desktopBgColor_.name());
  settings.setValue("FgColor", desktopFgColor_.name());
  settings.setValue("ShadowColor", desktopShadowColor_.name());
  // desktop_font=Sans 12
  // bool showWmMenu;
  settings.setValue("ShowHidden", desktopShowHidden_);
  settings.setValue("SortOrder", desktopSortOrder_);
  settings.setValue("SortColumn", (int)desktopSortColumn_);
  settings.endGroup();

  settings.beginGroup("Volume");
  settings.setValue("MountOnStartup", mountOnStartup_);
  settings.setValue("MountRemovable", mountRemovable_);
  settings.setValue("AutoRun", autoRun_);
  settings.endGroup();

  settings.beginGroup("FolderView");
  settings.setValue("Mode", viewMode_);
  settings.setValue("ShowHidden", showHidden_);
  settings.setValue("SortOrder", sortOrder_);
  settings.setValue("SortColumn", (int)sortColumn_);

  // override config in libfm's FmConfig
  settings.setValue("BigIconSize", bigIconSize_);
  settings.setValue("SmallIconSize", smallIconSize_);
  settings.setValue("SidePaneIconSize", sidePaneIconSize_);
  settings.setValue("ThumbnailIconSize", thumbnailIconSize_);
  settings.endGroup();
  
  settings.beginGroup("Window");
  settings.setValue("Width", windowWidth_);
  settings.setValue("Height", windowHeight_);
  settings.setValue("AlwaysShowTabs", alwaysShowTabs_);
  settings.setValue("ShowTabClose", showTabClose_);
  settings.setValue("SplitterPos", splitterPos_);
  settings.setValue("SidePaneMode", sidePaneMode_);
  settings.endGroup();

}

#include "settings.moc"
