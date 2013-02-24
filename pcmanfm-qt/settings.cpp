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
  desktopSortColumn_(Fm::FolderModel::ColumnName),
  alwaysShowTabs_(true),
  windowWidth_(640),
  windowHeight_(480),
  splitterPos_(120),
  sidePaneMode_(0),
  viewMode_(Fm::FolderView::IconMode),
  showHidden_(false),
  sortOrder_(Qt::AscendingOrder),
  sortColumn_(Fm::FolderModel::ColumnName),
  // settings for use with libfm
  singleClick_(false),
  useTrash_(true),
  confirmDelete_(true),
  // bool thumbnailLocal_;
  // bool thumbnailMax;
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
  settings.endGroup();

  settings.beginGroup("Behavior");
  bookmarkOpenMethod_ = settings.value("BookmarkOpenMethod").toInt();
  // settings for use with libfm
  useTrash_ = settings.value("UseTrash").toBool();
  singleClick_ = settings.value("SingleClick").toBool();
  confirmDelete_ = settings.value("ConfirmDelete").toBool();
  // bool thumbnailLocal_;
  // bool thumbnailMax;
  settings.endGroup();

  settings.beginGroup("Desktop");
  wallpaperMode_ = settings.value("WallpaperMode").toInt();
  wallpaper_ = settings.value("Wallpaper").toString();
  desktopBgColor_.setNamedColor(settings.value("BgColor").toString());
  desktopFgColor_.setNamedColor(settings.value("FgColor").toString());
  desktopShadowColor_.setNamedColor(settings.value("ShadowColor").toString());
  // desktop_font=Sans 12
  // bool showWmMenu;
  desktopShowHidden_ = settings.value("ShowHidden").toBool();
  desktopSortOrder_ = settings.value("SortOrder").toInt();
  desktopSortColumn_ = settings.value("SortColumn").toInt();
  settings.endGroup();

  settings.beginGroup("Volume");
  mountOnStartup_ = settings.value("MountOnStartup").toBool();
  mountRemovable_ = settings.value("MountRemovable").toBool();
  autoRun_ = settings.value("AutoRun").toBool();
  settings.endGroup();

  settings.beginGroup("FolderView");
  viewMode_ = settings.value("Mode").toInt();
  showHidden_ = settings.value("ShowHidden").toBool();
  sortOrder_ = settings.value("SortOrder").toInt();
  sortColumn_ = settings.value("SortColumn").toInt();

  // override config in libfm's FmConfig
  bigIconSize_ = settings.value("BigIconSize").toInt();
  smallIconSize_ = settings.value("SmallIconSize_").toInt();
  sidePaneIconSize_ = settings.value("SidePaneIconSize_").toInt();
  thumbnailIconSize_ = settings.value("ThumbnailIconSize_").toInt();
  settings.endGroup();
  
  settings.beginGroup("Window");
  windowWidth_ = settings.value("Width").toInt();
  windowHeight_ = settings.value("Height").toInt();
  alwaysShowTabs_ = settings.value("AlwaysShowTabs").toBool();
  splitterPos_ = settings.value("SplitterPos").toInt();
  sidePaneMode_ = settings.value("SidePaneMode").toInt();
  settings.endGroup();

}

bool Settings::saveFile(QString filePath) {
  QSettings settings(filePath, QSettings::IniFormat);

  settings.beginGroup("System");
  settings.setValue("IconThemeName", iconThemeName_);
  settings.setValue("SuCommand", suCommand_);
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
  settings.setValue("SmallIconSize_", smallIconSize_);
  settings.setValue("SidePaneIconSize_", sidePaneIconSize_);
  settings.setValue("ThumbnailIconSize_", thumbnailIconSize_);
  settings.endGroup();
  
  settings.beginGroup("Window");
  settings.setValue("Width", windowWidth_);
  settings.setValue("Height", windowHeight_);
  settings.setValue("AlwaysShowTabs", alwaysShowTabs_);
  settings.setValue("SplitterPos", splitterPos_);
  settings.setValue("SidePaneMode", sidePaneMode_);
  settings.endGroup();

}

#include "settings.moc"
