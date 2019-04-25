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


#include "settings.h"
#include <QDir>
#include <QFile>
#include <QStringBuilder>
#include <QSettings>
#include <QApplication>
#include "desktopwindow.h"
#include <libfm-qt/utilities.h>
#include <libfm-qt/core/folderconfig.h>
#include <libfm-qt/core/terminal.h>
#include <QStandardPaths>

namespace PCManFM {

inline static const char* bookmarkOpenMethodToString(OpenDirTargetType value);
inline static OpenDirTargetType bookmarkOpenMethodFromString(const QString str);

inline static const char* wallpaperModeToString(int value);
inline static int wallpaperModeFromString(const QString str);

inline static const char* viewModeToString(Fm::FolderView::ViewMode value);
inline static Fm::FolderView::ViewMode viewModeFromString(const QString str);

inline static const char* sidePaneModeToString(Fm::SidePane::Mode value);
inline static Fm::SidePane::Mode sidePaneModeFromString(const QString& str);

inline static const char* sortOrderToString(Qt::SortOrder order);
inline static Qt::SortOrder sortOrderFromString(const QString str);

inline static const char* sortColumnToString(Fm::FolderModel::ColumnId value);
inline static Fm::FolderModel::ColumnId sortColumnFromString(const QString str);

Settings::Settings():
    QObject(),
    supportTrash_(Fm::uriExists("trash:///")), // check if trash:/// is supported
    fallbackIconThemeName_(),
    useFallbackIconTheme_(QIcon::themeName().isEmpty() || QIcon::themeName() == "hicolor"),
    bookmarkOpenMethod_(OpenInCurrentTab),
    suCommand_(),
    terminal_(),
    mountOnStartup_(true),
    mountRemovable_(true),
    autoRun_(true),
    closeOnUnmount_(false),
    wallpaperMode_(0),
    wallpaper_(),
    wallpaperDialogSize_(QSize(700, 500)),
    wallpaperDialogSplitterPos_(200),
    lastSlide_(),
    wallpaperDir_(),
    slideShowInterval_(0),
    wallpaperRandomize_(false),
    desktopBgColor_(),
    desktopFgColor_(),
    desktopShadowColor_(),
    desktopIconSize_(48),
    desktopShowHidden_(false),
    desktopHideItems_(false),
    desktopSortOrder_(Qt::AscendingOrder),
    desktopSortColumn_(Fm::FolderModel::ColumnFileMTime),
    desktopSortFolderFirst_(true),
    alwaysShowTabs_(true),
    showTabClose_(true),
    rememberWindowSize_(true),
    fixedWindowWidth_(640),
    fixedWindowHeight_(480),
    lastWindowWidth_(640),
    lastWindowHeight_(480),
    lastWindowMaximized_(false),
    splitterPos_(120),
    sidePaneMode_(Fm::SidePane::ModePlaces),
    showMenuBar_(true),
    splitView_(false),
    viewMode_(Fm::FolderView::IconMode),
    showHidden_(false),
    sortOrder_(Qt::AscendingOrder),
    sortColumn_(Fm::FolderModel::ColumnFileName),
    sortFolderFirst_(true),
    sortCaseSensitive_(false),
    showFilter_(false),
    pathBarButtons_(true),
    // settings for use with libfm
    singleClick_(false),
    autoSelectionDelay_(600),
    useTrash_(true),
    confirmDelete_(true),
    noUsbTrash_(false),
    confirmTrash_(false),
    quickExec_(false),
    selectNewFiles_(false),
    showThumbnails_(true),
    archiver_(),
    siUnit_(false),
    placesHome_(true),
    placesDesktop_(true),
    placesApplications_(true),
    placesTrash_(true),
    placesRoot_(true),
    placesComputer_(true),
    placesNetwork_(true),
    bigIconSize_(48),
    smallIconSize_(24),
    sidePaneIconSize_(24),
    thumbnailIconSize_(128),
    folderViewCellMargins_(QSize(3, 3)),
    desktopCellMargins_(QSize(3, 1)),
    searchNameCaseInsensitive_(false),
    searchContentCaseInsensitive_(false),
    searchNameRegexp_(true),
    searchContentRegexp_(true),
    searchRecursive_(false),
    searchhHidden_(false) {
}

Settings::~Settings() {

}

QString Settings::xdgUserConfigDir() {
    QString dirName;
    // WARNING: Don't use XDG_CONFIG_HOME with root because it might
    // give the user config directory if gksu-properties is set to su.
    if(geteuid() != 0) { // non-root user
        dirName = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    }
    if(dirName.isEmpty()) {
        dirName = QDir::homePath() % QLatin1String("/.config");
    }
    return dirName;
}

QString Settings::profileDir(QString profile, bool useFallback) {
    // try user-specific config file first
    QString dirName = xdgUserConfigDir();
    dirName = dirName % "/pcmanfm-qt/" % profile;
    QDir dir(dirName);

    // if user config dir does not exist, try system-wide config dirs instead
    if(!dir.exists() && useFallback) {
        QString fallbackDir;
        for(const char* const* configDir = g_get_system_config_dirs(); *configDir; ++configDir) {
            fallbackDir = QString(*configDir) % "/pcmanfm-qt/" % profile;
            dir.setPath(fallbackDir);
            if(dir.exists()) {
                dirName = fallbackDir;
                break;
            }
        }
    }
    return dirName;
}

bool Settings::load(QString profile) {
    profileName_ = profile;
    QString fileName = profileDir(profile, true) % "/settings.conf";
    return loadFile(fileName);
}

bool Settings::save(QString profile) {
    QString fileName = profileDir(profile.isEmpty() ? profileName_ : profile) % "/settings.conf";
    return saveFile(fileName);
}

bool Settings::loadFile(QString filePath) {
    QSettings settings(filePath, QSettings::IniFormat);
    settings.beginGroup("System");
    fallbackIconThemeName_ = settings.value("FallbackIconThemeName").toString();
    if(fallbackIconThemeName_.isEmpty()) {
        // FIXME: we should choose one from installed icon themes or get
        // the value from XSETTINGS instead of hard code a fallback value.
        fallbackIconThemeName_ = "oxygen"; // fallback icon theme name
    }
    suCommand_ = settings.value("SuCommand", "lxqt-sudo %s").toString();
    setTerminal(settings.value("Terminal", "xterm").toString());
    setArchiver(settings.value("Archiver", "file-roller").toString());
    setSiUnit(settings.value("SIUnit", false).toBool());

    setOnlyUserTemplates(settings.value("OnlyUserTemplates", false).toBool());
    setTemplateTypeOnce(settings.value("TemplateTypeOnce", false).toBool());
    setTemplateRunApp(settings.value("TemplateRunApp", false).toBool());

    settings.endGroup();

    settings.beginGroup("Behavior");
    bookmarkOpenMethod_ = bookmarkOpenMethodFromString(settings.value("BookmarkOpenMethod").toString());
    // settings for use with libfm
    useTrash_ = settings.value("UseTrash", true).toBool();
    singleClick_ = settings.value("SingleClick", false).toBool();
    autoSelectionDelay_ = settings.value("AutoSelectionDelay", 600).toInt();
    confirmDelete_ = settings.value("ConfirmDelete", true).toBool();
    setNoUsbTrash(settings.value("NoUsbTrash", false).toBool());
    confirmTrash_ = settings.value("ConfirmTrash", false).toBool();
    setQuickExec(settings.value("QuickExec", false).toBool());
    selectNewFiles_ = settings.value("SelectNewFiles", false).toBool();
    // bool thumbnailLocal_;
    // bool thumbnailMax;
    settings.endGroup();

    settings.beginGroup("Desktop");
    wallpaperMode_ = wallpaperModeFromString(settings.value("WallpaperMode").toString());
    wallpaper_ = settings.value("Wallpaper").toString();
    wallpaperDialogSize_ = settings.value("WallpaperDialogSize", QSize(700, 500)).toSize();
    wallpaperDialogSplitterPos_ = settings.value("WallpaperDialogSplitterPos", 200).toInt();
    lastSlide_ = settings.value("LastSlide").toString();
    wallpaperDir_ = settings.value("WallpaperDirectory").toString();
    slideShowInterval_ = settings.value("SlideShowInterval", 0).toInt();
    wallpaperRandomize_ = settings.value("WallpaperRandomize").toBool();
    desktopBgColor_.setNamedColor(settings.value("BgColor", "#000000").toString());
    desktopFgColor_.setNamedColor(settings.value("FgColor", "#ffffff").toString());
    desktopShadowColor_.setNamedColor(settings.value("ShadowColor", "#000000").toString());
    if(settings.contains("Font")) {
        desktopFont_.fromString(settings.value("Font").toString());
    }
    else {
        desktopFont_ = QApplication::font();
    }
    desktopIconSize_ = settings.value("DesktopIconSize", 48).toInt();
    desktopShortcuts_ = settings.value("DesktopShortcuts").toStringList();
    desktopShowHidden_ = settings.value("ShowHidden", false).toBool();
    desktopHideItems_ = settings.value("HideItems", false).toBool();

    desktopSortOrder_ = sortOrderFromString(settings.value("SortOrder").toString());
    desktopSortColumn_ = sortColumnFromString(settings.value("SortColumn").toString());
    desktopSortFolderFirst_ = settings.value("SortFolderFirst", true).toBool();

    desktopCellMargins_ = (settings.value("DesktopCellMargins", QSize(3, 1)).toSize()
                           .expandedTo(QSize(0, 0))).boundedTo(QSize(48, 48));
    settings.endGroup();

    settings.beginGroup("Volume");
    mountOnStartup_ = settings.value("MountOnStartup", true).toBool();
    mountRemovable_ = settings.value("MountRemovable", true).toBool();
    autoRun_ = settings.value("AutoRun", true).toBool();
    closeOnUnmount_ = settings.value("CloseOnUnmount", true).toBool();
    settings.endGroup();

    settings.beginGroup("Thumbnail");
    showThumbnails_ = settings.value("ShowThumbnails", true).toBool();
    setMaxThumbnailFileSize(settings.value("MaxThumbnailFileSize", 4096).toInt());
    setThumbnailLocalFilesOnly(settings.value("ThumbnailLocalFilesOnly", true).toBool());
    settings.endGroup();

    settings.beginGroup("FolderView");
    viewMode_ = viewModeFromString(settings.value("Mode", Fm::FolderView::IconMode).toString());
    showHidden_ = settings.value("ShowHidden", false).toBool();
    sortOrder_ = sortOrderFromString(settings.value("SortOrder").toString());
    sortColumn_ = sortColumnFromString(settings.value("SortColumn").toString());
    sortFolderFirst_ = settings.value("SortFolderFirst", true).toBool();
    sortCaseSensitive_ = settings.value("SortCaseSensitive", false).toBool();
    showFilter_ = settings.value("ShowFilter", false).toBool();

    setBackupAsHidden(settings.value("BackupAsHidden", false).toBool());
    showFullNames_ = settings.value("ShowFullNames", false).toBool();
    shadowHidden_ = settings.value("ShadowHidden", false).toBool();

    // override config in libfm's FmConfig
    bigIconSize_ = toIconSize(settings.value("BigIconSize", 48).toInt(), Big);
    smallIconSize_ = toIconSize(settings.value("SmallIconSize", 24).toInt(), Small);
    sidePaneIconSize_ = toIconSize(settings.value("SidePaneIconSize", 24).toInt(), Small);
    thumbnailIconSize_ = toIconSize(settings.value("ThumbnailIconSize", 128).toInt(), Thumbnail);

    folderViewCellMargins_ = (settings.value("FolderViewCellMargins", QSize(3, 3)).toSize()
                              .expandedTo(QSize(0, 0))).boundedTo(QSize(48, 48));

    // detailed list columns
    customColumnWidths_ = settings.value("CustomColumnWidths").toList();
    hiddenColumns_ = settings.value("HiddenColumns").toList();

    settings.endGroup();

    settings.beginGroup("Places");
    placesHome_ = settings.value("PlacesHome", true).toBool();
    placesDesktop_ = settings.value("PlacesDesktop", true).toBool();
    placesApplications_ = settings.value("PlacesApplications", true).toBool();
    placesTrash_ = settings.value("PlacesTrash", true).toBool();
    placesRoot_ = settings.value("PlacesRoot", true).toBool();
    placesComputer_ = settings.value("PlacesComputer", true).toBool();
    placesNetwork_ = settings.value("PlacesNetwork", true).toBool();
    hiddenPlaces_ = settings.value("HiddenPlaces").toStringList().toSet();
    settings.endGroup();

    settings.beginGroup("Window");
    fixedWindowWidth_ = settings.value("FixedWidth", 640).toInt();
    fixedWindowHeight_ = settings.value("FixedHeight", 480).toInt();
    lastWindowWidth_ = settings.value("LastWindowWidth", 640).toInt();
    lastWindowHeight_ = settings.value("LastWindowHeight", 480).toInt();
    lastWindowMaximized_ = settings.value("LastWindowMaximized", false).toBool();
    rememberWindowSize_ = settings.value("RememberWindowSize", true).toBool();
    alwaysShowTabs_ = settings.value("AlwaysShowTabs", true).toBool();
    showTabClose_ = settings.value("ShowTabClose", true).toBool();
    splitterPos_ = settings.value("SplitterPos", 150).toInt();
    sidePaneMode_ = sidePaneModeFromString(settings.value("SidePaneMode").toString());
    showMenuBar_ = settings.value("ShowMenuBar", true).toBool();
    splitView_ = settings.value("SplitView", false).toBool();
    pathBarButtons_ = settings.value("PathBarButtons", true).toBool();
    settings.endGroup();

    settings.beginGroup("Search");
    searchNameCaseInsensitive_ = settings.value("searchNameCaseInsensitive", false).toBool();
    searchContentCaseInsensitive_ = settings.value("searchContentCaseInsensitive", false).toBool();
    searchNameRegexp_ = settings.value("searchNameRegexp", true).toBool();
    searchContentRegexp_ = settings.value("searchContentRegexp", true).toBool();
    searchRecursive_ = settings.value("searchRecursive", false).toBool();
    searchhHidden_ = settings.value("searchhHidden", false).toBool();
    settings.endGroup();

    return true;
}

bool Settings::saveFile(QString filePath) {
    QSettings settings(filePath, QSettings::IniFormat);

    settings.beginGroup("System");
    settings.setValue("FallbackIconThemeName", fallbackIconThemeName_);
    settings.setValue("SuCommand", suCommand_);
    settings.setValue("Terminal", terminal_);
    settings.setValue("Archiver", archiver_);
    settings.setValue("SIUnit", siUnit_);

    settings.setValue("OnlyUserTemplates", onlyUserTemplates_);
    settings.setValue("TemplateTypeOnce", templateTypeOnce_);
    settings.setValue("TemplateRunApp", templateRunApp_);

    settings.endGroup();

    settings.beginGroup("Behavior");
    settings.setValue("BookmarkOpenMethod", bookmarkOpenMethodToString(bookmarkOpenMethod_));
    // settings for use with libfm
    settings.setValue("UseTrash", useTrash_);
    settings.setValue("SingleClick", singleClick_);
    settings.setValue("AutoSelectionDelay", autoSelectionDelay_);
    settings.setValue("ConfirmDelete", confirmDelete_);
    settings.setValue("NoUsbTrash", noUsbTrash_);
    settings.setValue("ConfirmTrash", confirmTrash_);
    settings.setValue("QuickExec", quickExec_);
    settings.setValue("SelectNewFiles", selectNewFiles_);
    // bool thumbnailLocal_;
    // bool thumbnailMax;
    settings.endGroup();

    settings.beginGroup("Desktop");
    settings.setValue("WallpaperMode", wallpaperModeToString(wallpaperMode_));
    settings.setValue("Wallpaper", wallpaper_);
    settings.setValue("WallpaperDialogSize", wallpaperDialogSize_);
    settings.setValue("WallpaperDialogSplitterPos", wallpaperDialogSplitterPos_);
    settings.setValue("LastSlide", lastSlide_);
    settings.setValue("WallpaperDirectory", wallpaperDir_);
    settings.setValue("SlideShowInterval", slideShowInterval_);
    settings.setValue("WallpaperRandomize", wallpaperRandomize_);
    settings.setValue("BgColor", desktopBgColor_.name());
    settings.setValue("FgColor", desktopFgColor_.name());
    settings.setValue("ShadowColor", desktopShadowColor_.name());
    settings.setValue("Font", desktopFont_.toString());
    settings.setValue("DesktopIconSize", desktopIconSize_);
    settings.setValue("DesktopShortcuts", desktopShortcuts_);
    settings.setValue("ShowHidden", desktopShowHidden_);
    settings.setValue("HideItems", desktopHideItems_);
    settings.setValue("SortOrder", sortOrderToString(desktopSortOrder_));
    settings.setValue("SortColumn", sortColumnToString(desktopSortColumn_));
    settings.setValue("SortFolderFirst", desktopSortFolderFirst_);
    settings.setValue("DesktopCellMargins", desktopCellMargins_);
    settings.endGroup();

    settings.beginGroup("Volume");
    settings.setValue("MountOnStartup", mountOnStartup_);
    settings.setValue("MountRemovable", mountRemovable_);
    settings.setValue("AutoRun", autoRun_);
    settings.setValue("CloseOnUnmount", closeOnUnmount_);
    settings.endGroup();

    settings.beginGroup("Thumbnail");
    settings.setValue("ShowThumbnails", showThumbnails_);
    settings.setValue("MaxThumbnailFileSize", maxThumbnailFileSize());
    settings.setValue("ThumbnailLocalFilesOnly", thumbnailLocalFilesOnly());
    settings.endGroup();

    settings.beginGroup("FolderView");
    settings.setValue("Mode", viewModeToString(viewMode_));
    settings.setValue("ShowHidden", showHidden_);
    settings.setValue("SortOrder", sortOrderToString(sortOrder_));
    settings.setValue("SortColumn", sortColumnToString(sortColumn_));
    settings.setValue("SortFolderFirst", sortFolderFirst_);
    settings.setValue("SortCaseSensitive", sortCaseSensitive_);
    settings.setValue("ShowFilter", showFilter_);

    settings.setValue("BackupAsHidden", backupAsHidden_);
    settings.setValue("ShowFullNames", showFullNames_);
    settings.setValue("ShadowHidden", shadowHidden_);

    // override config in libfm's FmConfig
    settings.setValue("BigIconSize", bigIconSize_);
    settings.setValue("SmallIconSize", smallIconSize_);
    settings.setValue("SidePaneIconSize", sidePaneIconSize_);
    settings.setValue("ThumbnailIconSize", thumbnailIconSize_);

    settings.setValue("FolderViewCellMargins", folderViewCellMargins_);

    // detailed list columns
    settings.setValue("CustomColumnWidths", customColumnWidths_);
    std::sort(hiddenColumns_.begin(), hiddenColumns_.end());
    settings.setValue("HiddenColumns", hiddenColumns_);

    settings.endGroup();

    settings.beginGroup("Places");
    settings.setValue("PlacesHome", placesHome_);
    settings.setValue("PlacesDesktop", placesDesktop_);
    settings.setValue("PlacesApplications", placesApplications_);
    settings.setValue("PlacesTrash", placesTrash_);
    settings.setValue("PlacesRoot", placesRoot_);
    settings.setValue("PlacesComputer", placesComputer_);
    settings.setValue("PlacesNetwork", placesNetwork_);
    if (hiddenPlaces_.isEmpty()) {  // don't save "@Invalid()"
        settings.remove("HiddenPlaces");
    }
    else {
        QStringList hiddenPlaces = hiddenPlaces_.toList();
        settings.setValue("HiddenPlaces", hiddenPlaces);
    }
    settings.endGroup();

    settings.beginGroup("Window");
    settings.setValue("FixedWidth", fixedWindowWidth_);
    settings.setValue("FixedHeight", fixedWindowHeight_);
    settings.setValue("LastWindowWidth", lastWindowWidth_);
    settings.setValue("LastWindowHeight", lastWindowHeight_);
    settings.setValue("LastWindowMaximized", lastWindowMaximized_);
    settings.setValue("RememberWindowSize", rememberWindowSize_);
    settings.setValue("AlwaysShowTabs", alwaysShowTabs_);
    settings.setValue("ShowTabClose", showTabClose_);
    settings.setValue("SplitterPos", splitterPos_);
    settings.setValue("SidePaneMode", sidePaneModeToString(sidePaneMode_));
    settings.setValue("ShowMenuBar", showMenuBar_);
    settings.setValue("SplitView", splitView_);
    settings.setValue("PathBarButtons", pathBarButtons_);
    settings.endGroup();

    // save per-folder settings
    Fm::FolderConfig::saveCache();

    settings.beginGroup("Search");
    settings.setValue("searchNameCaseInsensitive", searchNameCaseInsensitive_);
    settings.setValue("searchContentCaseInsensitive", searchContentCaseInsensitive_);
    settings.setValue("searchNameRegexp", searchNameRegexp_);
    settings.setValue("searchContentRegexp", searchContentRegexp_);
    settings.setValue("searchRecursive", searchRecursive_);
    settings.setValue("searchhHidden", searchhHidden_);
    settings.endGroup();

    return true;
}

const QList<int> & Settings::iconSizes(IconType type) {
    static const QList<int> sizes_big = {96, 72, 64, 48, 32};
    static const QList<int> sizes_thumbnail = {256, 224, 192, 160, 128, 96, 64};
    static const QList<int> sizes_small = {48, 32, 24, 22, 16};
    switch(type) {
    case Big:
        return sizes_big;
        break;
    case Thumbnail:
        return sizes_thumbnail;
        break;
    case Small:
    default:
        return sizes_small;
        break;
    }
}

int Settings::toIconSize(int size, IconType type) const {
    const QList<int> & sizes = iconSizes(type);
    for (const auto & s : sizes) {
        if(size >= s) {
            return s;
        }
    }
    return sizes.back();
}

static const char* bookmarkOpenMethodToString(OpenDirTargetType value) {
    switch(value) {
    case OpenInCurrentTab:
    default:
        return "current_tab";
    case OpenInNewTab:
        return "new_tab";
    case OpenInNewWindow:
        return "new_window";
    case OpenInLastActiveWindow:
        return "last_window";
    }
    return "";
}

static OpenDirTargetType bookmarkOpenMethodFromString(const QString str) {

    if(str == QStringLiteral("new_tab")) {
        return OpenInNewTab;
    }
    else if(str == QStringLiteral("new_window")) {
        return OpenInNewWindow;
    }
    else if(str == QStringLiteral("last_window")) {
        return OpenInLastActiveWindow;
    }
    return OpenInCurrentTab;
}

static const char* viewModeToString(Fm::FolderView::ViewMode value) {
    const char* ret;
    switch(value) {
    case Fm::FolderView::IconMode:
    default:
        ret = "icon";
        break;
    case Fm::FolderView::CompactMode:
        ret = "compact";
        break;
    case Fm::FolderView::DetailedListMode:
        ret = "detailed";
        break;
    case Fm::FolderView::ThumbnailMode:
        ret = "thumbnail";
        break;
    }
    return ret;
}

Fm::FolderView::ViewMode viewModeFromString(const QString str) {
    Fm::FolderView::ViewMode ret;
    if(str == "icon") {
        ret = Fm::FolderView::IconMode;
    }
    else if(str == "compact") {
        ret = Fm::FolderView::CompactMode;
    }
    else if(str == "detailed") {
        ret = Fm::FolderView::DetailedListMode;
    }
    else if(str == "thumbnail") {
        ret = Fm::FolderView::ThumbnailMode;
    }
    else {
        ret = Fm::FolderView::IconMode;
    }
    return ret;
}

static const char* sortOrderToString(Qt::SortOrder order) {
    return (order == Qt::DescendingOrder ? "descending" : "ascending");
}

static Qt::SortOrder sortOrderFromString(const QString str) {
    return (str == "descending" ? Qt::DescendingOrder : Qt::AscendingOrder);
}

static const char* sortColumnToString(Fm::FolderModel::ColumnId value) {
    const char* ret;
    switch(value) {
    case Fm::FolderModel::ColumnFileName:
    default:
        ret = "name";
        break;
    case Fm::FolderModel::ColumnFileType:
        ret = "type";
        break;
    case Fm::FolderModel::ColumnFileSize:
        ret = "size";
        break;
    case Fm::FolderModel::ColumnFileMTime:
        ret = "mtime";
        break;
    case Fm::FolderModel::ColumnFileOwner:
        ret = "owner";
        break;
    case Fm::FolderModel::ColumnFileGroup:
        ret = "group";
        break;
    }
    return ret;
}

static Fm::FolderModel::ColumnId sortColumnFromString(const QString str) {
    Fm::FolderModel::ColumnId ret;
    if(str == "name") {
        ret = Fm::FolderModel::ColumnFileName;
    }
    else if(str == "type") {
        ret = Fm::FolderModel::ColumnFileType;
    }
    else if(str == "size") {
        ret = Fm::FolderModel::ColumnFileSize;
    }
    else if(str == "mtime") {
        ret = Fm::FolderModel::ColumnFileMTime;
    }
    else if(str == "owner") {
        ret = Fm::FolderModel::ColumnFileOwner;
    }
    else if(str == "group") {
        ret = Fm::FolderModel::ColumnFileGroup;
    }
    else {
        ret = Fm::FolderModel::ColumnFileName;
    }
    return ret;
}

static const char* wallpaperModeToString(int value) {
    const char* ret;
    switch(value) {
    case DesktopWindow::WallpaperNone:
    default:
        ret = "none";
        break;
    case DesktopWindow::WallpaperStretch:
        ret = "stretch";
        break;
    case DesktopWindow::WallpaperFit:
        ret = "fit";
        break;
    case DesktopWindow::WallpaperCenter:
        ret = "center";
        break;
    case DesktopWindow::WallpaperTile:
        ret = "tile";
        break;
    case DesktopWindow::WallpaperZoom:
        ret = "zoom";
        break;
    }
    return ret;
}

static int wallpaperModeFromString(const QString str) {
    int ret;
    if(str == "stretch") {
        ret = DesktopWindow::WallpaperStretch;
    }
    else if(str == "fit") {
        ret = DesktopWindow::WallpaperFit;
    }
    else if(str == "center") {
        ret = DesktopWindow::WallpaperCenter;
    }
    else if(str == "tile") {
        ret = DesktopWindow::WallpaperTile;
    }
    else if(str == "zoom") {
        ret = DesktopWindow::WallpaperZoom;
    }
    else {
        ret = DesktopWindow::WallpaperNone;
    }
    return ret;
}

static const char* sidePaneModeToString(Fm::SidePane::Mode value) {
    const char* ret;
    switch(value) {
    case Fm::SidePane::ModePlaces:
    default:
        ret = "places";
        break;
    case Fm::SidePane::ModeDirTree:
        ret = "dirtree";
        break;
    case Fm::SidePane::ModeNone:
        ret = "none";
        break;
    }
    return ret;
}

static Fm::SidePane::Mode sidePaneModeFromString(const QString& str) {
    Fm::SidePane::Mode ret;
    if(str == "none") {
        ret = Fm::SidePane::ModeNone;
    }
    else if(str == "dirtree") {
        ret = Fm::SidePane::ModeDirTree;
    }
    else {
        ret = Fm::SidePane::ModePlaces;
    }
    return ret;
}

void Settings::setTerminal(QString terminalCommand) {
    terminal_ = terminalCommand;
    Fm::setDefaultTerminal(terminal_.toStdString());
}


// per-folder settings
FolderSettings Settings::loadFolderSettings(const Fm::FilePath& path) const {
    FolderSettings settings;
    Fm::FolderConfig cfg(path);
    // set defaults
    settings.setSortOrder(sortOrder());
    settings.setSortColumn(sortColumn());
    settings.setViewMode(viewMode());
    settings.setShowHidden(showHidden());
    settings.setSortFolderFirst(sortFolderFirst());
    settings.setSortCaseSensitive(sortCaseSensitive());
    // columns?
    if(!cfg.isEmpty()) {
        // load folder-specific settings
        settings.setCustomized(true);

        char* str;
        // load sorting
        str = cfg.getString("SortOrder");
        if(str != nullptr) {
            settings.setSortOrder(sortOrderFromString(str));
            g_free(str);
        }

        str = cfg.getString("SortColumn");
        if(str != nullptr) {
            settings.setSortColumn(sortColumnFromString(str));
            g_free(str);
        }

        str = cfg.getString("ViewMode");
        if(str != nullptr) {
            // set view mode
            settings.setViewMode(viewModeFromString(str));
            g_free(str);
        }

        bool show_hidden;
        if(cfg.getBoolean("ShowHidden", &show_hidden)) {
            settings.setShowHidden(show_hidden);
        }

        bool folder_first;
        if(cfg.getBoolean("SortFolderFirst", &folder_first)) {
            settings.setSortFolderFirst(folder_first);
        }

        bool case_sensitive;
        if(cfg.getBoolean("SortCaseSensitive", &case_sensitive)) {
            settings.setSortCaseSensitive(case_sensitive);
        }
    }
    return settings;
}

void Settings::saveFolderSettings(const Fm::FilePath& path, const FolderSettings& settings) {
    if(path) {
        // ensure that we have the libfm dir
        QString dirName = xdgUserConfigDir() % QStringLiteral("/libfm");
        QDir().mkpath(dirName);  // if libfm config dir does not exist, create it

        Fm::FolderConfig cfg(path);
        cfg.setString("SortOrder", sortOrderToString(settings.sortOrder()));
        cfg.setString("SortColumn", sortColumnToString(settings.sortColumn()));
        cfg.setString("ViewMode", viewModeToString(settings.viewMode()));
        cfg.setBoolean("ShowHidden", settings.showHidden());
        cfg.setBoolean("SortFolderFirst", settings.sortFolderFirst());
        cfg.setBoolean("SortCaseSensitive", settings.sortCaseSensitive());
    }
}

void Settings::clearFolderSettings(const Fm::FilePath& path) const {
    if(path) {
        Fm::FolderConfig cfg(path);
        cfg.purge();
    }
}


} // namespace PCManFM
