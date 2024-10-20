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


#include "application.h"
#include "mainwindow.h"
#include "desktopwindow.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDir>
#include <QVector>
#include <QLocale>
#include <QLibraryInfo>
#include <QFile>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QSocketNotifier>
#include <QScreen>
#include <QWindow>
#include <QFileSystemWatcher>

#include <gio/gio.h>
#include <sys/socket.h>

#include <libfm-qt6/mountoperation.h>
#include <libfm-qt6/filepropsdialog.h>
#include <libfm-qt6/filesearchdialog.h>
#include <libfm-qt6/core/terminal.h>
#include <libfm-qt6/core/bookmarks.h>
#include <libfm-qt6/core/folderconfig.h>
#include <libfm-qt6/core/fileinfojob.h>

#include "applicationadaptor.h"
#include "applicationadaptorfreedesktopfilemanager.h"
#include "preferencesdialog.h"
#include "desktoppreferencesdialog.h"
#include "autorundialog.h"
#include "launcher.h"
#include "xdgdir.h"
#include "connectserverdialog.h"

#include <X11/Xlib.h>


namespace PCManFM {

static const char* serviceName = "org.pcmanfm.PCManFM";
static const char* ifaceName = "org.pcmanfm.Application";

int ProxyStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const {
    Application* app = static_cast<Application*>(qApp);
    if(hint == QStyle::SH_ItemView_ActivateItemOnSingleClick) {
        if (app->settings().singleClick()) {
            return true;
        }
        // follow the style
        return QCommonStyle::styleHint(hint,option,widget,returnData);
    }
    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

Application::Application(int& argc, char** argv):
    QApplication(argc, argv),
    libFm_(),
    settings_(),
    profileName_(QStringLiteral("default")),
    daemonMode_(false),
    enableDesktopManager_(false),
    desktopWindows_(),
    preferencesDialog_(),
    editBookmarksialog_(),
    volumeMonitor_(nullptr),
    userDirsWatcher_(nullptr),
    lxqtRunning_(false),
    openingLastTabs_(false) {

    argc_ = argc;
    argv_ = argv;

    setApplicationVersion(QStringLiteral(PCMANFM_QT_VERSION));

    underWayland_ = QGuiApplication::platformName() == QStringLiteral("wayland");

    // QDBusConnection::sessionBus().registerObject("/org/pcmanfm/Application", this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if(dbus.registerService(QLatin1String(serviceName))) {
        // we successfully registered the service
        isPrimaryInstance = true;
        setStyle(new ProxyStyle());
        //desktop()->installEventFilter(this);

        new ApplicationAdaptor(this);
        dbus.registerObject(QStringLiteral("/Application"), this);

        connect(this, &Application::aboutToQuit, this, &Application::onAboutToQuit);
        // aboutToQuit() is not signalled on SIGTERM, install signal handler
        installSigtermHandler();

        // Check if LXQt Session is running. LXQt has it's own Desktop Folder
        // editor. We just hide our editor when LXQt is running.
        QDBusInterface* lxqtSessionIface = new QDBusInterface(
            QStringLiteral("org.lxqt.session"),
            QStringLiteral("/LXQtSession"));
        if(lxqtSessionIface) {
            if(lxqtSessionIface->isValid()) {
                lxqtRunning_ = true;
                userDesktopFolder_ = XdgDir::readDesktopDir();
                initWatch();
            }
            delete lxqtSessionIface;
            lxqtSessionIface = nullptr;
        }


        // We also try to register the service "org.freedesktop.FileManager1".
        // We allow queuing of our request in case another file manager has already registered it.
        static const QString fileManagerService = QStringLiteral("org.freedesktop.FileManager1");
        connect(dbus.interface(), &QDBusConnectionInterface::serviceRegistered, this, [this](const QString& service) {
                if(fileManagerService == service) {
                    QDBusConnection dbus = QDBusConnection::sessionBus();
                    disconnect(dbus.interface(), &QDBusConnectionInterface::serviceRegistered, this, nullptr);
                    new ApplicationAdaptorFreeDesktopFileManager(this);
                    if(!dbus.registerObject(QStringLiteral("/org/freedesktop/FileManager1"), this)) {
                        qDebug() << "Can't register /org/freedesktop/FileManager1:" << dbus.lastError().message();
                    }
                }
        });
        dbus.interface()->registerService(fileManagerService, QDBusConnectionInterface::QueueService);
    }
    else {
        // an service of the same name is already registered.
        // we're not the first instance
        isPrimaryInstance = false;
    }
}

Application::~Application() {
    //desktop()->removeEventFilter(this);

    if(volumeMonitor_) {
        g_signal_handlers_disconnect_by_func(volumeMonitor_, gpointer(onVolumeAdded), this);
        g_object_unref(volumeMonitor_);
    }

    // if(enableDesktopManager_)
    //   removeNativeEventFilter(this);
}

void Application::initWatch() {
    QFile file_(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/user-dirs.dirs"));
    if(! file_.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << Q_FUNC_INFO << "Could not read: " << userDirsFile_;
        userDirsFile_ = QString();
    }
    else {
        userDirsFile_ = file_.fileName();
    }

    userDirsWatcher_ = new QFileSystemWatcher(this);
    userDirsWatcher_->addPath(userDirsFile_);
    connect(userDirsWatcher_, &QFileSystemWatcher::fileChanged, this, &Application::onUserDirsChanged);
}

bool Application::parseCommandLineArgs() {
    bool keepRunning = false;
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption profileOption(QStringList() << QStringLiteral("p") << QStringLiteral("profile"), tr("Name of configuration profile"), tr("PROFILE"));
    parser.addOption(profileOption);

    QCommandLineOption daemonOption(QStringList() << QStringLiteral("d") << QStringLiteral("daemon-mode"), tr("Run PCManFM-Qt as a daemon"));
    parser.addOption(daemonOption);

    QCommandLineOption quitOption(QStringList() << QStringLiteral("q") << QStringLiteral("quit"), tr("Quit PCManFM-Qt"));
    parser.addOption(quitOption);

    QCommandLineOption desktopOption(QStringLiteral("desktop"), tr("Launch desktop manager"));
    parser.addOption(desktopOption);

    QCommandLineOption desktopOffOption(QStringLiteral("desktop-off"), tr("Turn off desktop manager if it's running"));
    parser.addOption(desktopOffOption);

    QCommandLineOption desktopPrefOption(QStringLiteral("desktop-pref"), tr("Open desktop preference dialog on the page with the specified name") + QStringLiteral("\n") + tr("NAME") + QStringLiteral("=(general|bg|slide|advanced)"), tr("NAME"));
    parser.addOption(desktopPrefOption);

    QCommandLineOption newWindowOption(QStringList() << QStringLiteral("n") << QStringLiteral("new-window"), tr("Open new window"));
    parser.addOption(newWindowOption);

    QCommandLineOption findFilesOption(QStringList() << QStringLiteral("f") << QStringLiteral("find-files"), tr("Open Find Files utility"));
    parser.addOption(findFilesOption);

    QCommandLineOption setWallpaperOption(QStringList() << QStringLiteral("w") << QStringLiteral("set-wallpaper"), tr("Set desktop wallpaper from image FILE"), tr("FILE"));
    parser.addOption(setWallpaperOption);

    QCommandLineOption wallpaperModeOption(QStringLiteral("wallpaper-mode"), tr("Set mode of desktop wallpaper. MODE=(%1)").arg(QStringLiteral("color|stretch|fit|center|tile|zoom")), tr("MODE"));
    parser.addOption(wallpaperModeOption);

    QCommandLineOption showPrefOption(QStringLiteral("show-pref"), tr("Open Preferences dialog on the page with the specified name") + QStringLiteral("\n") + tr("NAME") + QStringLiteral("=(behavior|display|ui|thumbnail|volume|advanced)"), tr("NAME"));
    parser.addOption(showPrefOption);

    parser.addPositionalArgument(QStringLiteral("files"), tr("Files or directories to open"), tr("[FILE1, FILE2,...]"));

    parser.process(arguments());

    if(isPrimaryInstance) {
        qDebug("isPrimaryInstance");

        if(parser.isSet(daemonOption)) {
            daemonMode_ = true;
        }
        if(parser.isSet(profileOption)) {
            profileName_ = parser.value(profileOption);
        }

        // load app config
        settings_.load(profileName_);

        // init per-folder config
        QString perFolderConfigFile = settings_.profileDir(profileName_) + QStringLiteral("/dir-settings.conf");
        Fm::FolderConfig::init(perFolderConfigFile.toLocal8Bit().constData());


        if(settings_.useFallbackIconTheme()) {
            QIcon::setThemeName(settings_.fallbackIconThemeName());
        }

        // desktop icon management
        if(parser.isSet(desktopOption)) {
            desktopManager(true);
            keepRunning = true;
        }
        else if(parser.isSet(desktopOffOption)) {
            desktopManager(false);
        }

        if(parser.isSet(desktopPrefOption)) { // desktop preference dialog
            desktopPrefrences(parser.value(desktopPrefOption));
            keepRunning = true;
        }
        else if(parser.isSet(findFilesOption)) { // file searching utility
            findFiles(parser.positionalArguments());
            keepRunning = true;
        }
        else if(parser.isSet(showPrefOption)) { // preferences dialog
            preferences(parser.value(showPrefOption));
            keepRunning = true;
        }
        else if(parser.isSet(setWallpaperOption) || parser.isSet(wallpaperModeOption)) { // set wall paper
            setWallpaper(parser.value(setWallpaperOption), parser.value(wallpaperModeOption));
        }
        else {
            if(!parser.isSet(desktopOption) && !parser.isSet(desktopOffOption)) {
                bool reopenLastTabs = false;
                QStringList paths = parser.positionalArguments();
                if(paths.isEmpty()) {
                    // if no path is specified and we're using daemon mode,
                    // don't open current working directory
                    if(!daemonMode_) {
                        reopenLastTabs = true; // open last tab paths only if no path is specified
                        paths.push_back(QDir::currentPath());
                    }
                }
                if(!paths.isEmpty()) {
                    launchFiles(QDir::currentPath(), paths, parser.isSet(newWindowOption), reopenLastTabs);
                }
                keepRunning = true;
            }
        }
    }
    else {
        QDBusConnection dbus = QDBusConnection::sessionBus();
        QDBusInterface iface(QLatin1String(serviceName), QStringLiteral("/Application"), QLatin1String(ifaceName), dbus, this);
        if(parser.isSet(quitOption)) {
            iface.call(QStringLiteral("quit"));
            return false;
        }

        if(parser.isSet(desktopOption)) {
            iface.call(QStringLiteral("desktopManager"), true);
        }
        else if(parser.isSet(desktopOffOption)) {
            iface.call(QStringLiteral("desktopManager"), false);
        }

        if(parser.isSet(desktopPrefOption)) { // desktop preference dialog
            iface.call(QStringLiteral("desktopPrefrences"), parser.value(desktopPrefOption));
        }
        else if(parser.isSet(findFilesOption)) { // file searching utility
            iface.call(QStringLiteral("findFiles"), parser.positionalArguments());
        }
        else if(parser.isSet(showPrefOption)) { // preferences dialog
            iface.call(QStringLiteral("preferences"), parser.value(showPrefOption));
        }
        else if(parser.isSet(setWallpaperOption) || parser.isSet(wallpaperModeOption)) { // set wall paper
            iface.call(QStringLiteral("setWallpaper"), parser.value(setWallpaperOption), parser.value(wallpaperModeOption));
        }
        else {
            if(!parser.isSet(desktopOption) && !parser.isSet(desktopOffOption)) {
                bool reopenLastTabs = false;
                QStringList paths = parser.positionalArguments();
                if(paths.isEmpty()) {
                    reopenLastTabs = true; // open last tab paths only if no path is specified
                    paths.push_back(QDir::currentPath());
                }
                iface.call(QStringLiteral("launchFiles"), QDir::currentPath(), paths, parser.isSet(newWindowOption), reopenLastTabs);
            }
        }
    }
    return keepRunning;
}

void Application::init() {

    // install the translations built-into Qt itself
    if(qtTranslator.load(QStringLiteral("qt_") + QLocale::system().name(), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        installTranslator(&qtTranslator);
    }

    // install libfm-qt translator
    installTranslator(libFm_.translator());

    // install our own tranlations
    if(translator.load(QStringLiteral("pcmanfm-qt_") + QLocale::system().name(), QStringLiteral(PCMANFM_DATA_DIR) + QStringLiteral("/translations"))) {
        installTranslator(&translator);
    }
}

int Application::exec() {

    if(!parseCommandLineArgs()) {
        return 0;
    }

    if(daemonMode_) { // keep running even when there is no window opened.
        setQuitOnLastWindowClosed(false);
    }

    volumeMonitor_ = g_volume_monitor_get();
    // delay the volume manager a little because in newer versions of glib/gio there's a problem.
    // when the first volume monitor object is created, it discovers volumes asynchronously.
    // g_volume_monitor_get() immediately returns while the monitor is still discovering devices.
    // So initially g_volume_monitor_get_volumes() returns nothing, but shortly after that
    // we get volume-added signals for all of the volumes. This is not what we want.
    // So, we wait for 3 seconds here to let it finish device discovery.
    QTimer::singleShot(3000, this, &Application::initVolumeManager);

    return QCoreApplication::exec();
}


void Application::onUserDirsChanged() {
    qDebug() << Q_FUNC_INFO;
    bool file_deleted = !userDirsWatcher_->files().contains(userDirsFile_);
    if(file_deleted) {
        // if our config file is already deleted, reinstall a new watcher
        userDirsWatcher_->addPath(userDirsFile_);
    }

    const QString d = XdgDir::readDesktopDir();
    if(d != userDesktopFolder_) {
        userDesktopFolder_ = d;
        const QDir dir(d);
        if(dir.exists()) {
            const int N = desktopWindows_.size();
            for(int i = 0; i < N; ++i) {
                desktopWindows_.at(i)->setDesktopFolder();
            }
        }
        else {
            qWarning("Application::onUserDirsChanged: %s doesn't exist",
                     userDesktopFolder_.toUtf8().constData());
        }
    }
}

void Application::onAboutToQuit() {
    qDebug("aboutToQuit");

    settings_.save();
}

void Application::cleanPerFolderConfig() {
    // first save the perfolder config cache to have the list of all custom folders
    Fm::FolderConfig::saveCache();
    // then remove non-existent native folders from the list of custom folders
    QByteArray perFolderConfig = (settings_.profileDir(profileName_) + QStringLiteral("/dir-settings.conf"))
                                 .toLocal8Bit();
    GKeyFile* kf = g_key_file_new();
    if(g_key_file_load_from_file(kf, perFolderConfig.constData(), G_KEY_FILE_NONE, nullptr)) {
        bool removed(false);
        gchar **groups = g_key_file_get_groups(kf, nullptr);
        for(int i = 0; groups[i] != nullptr; i++) {
            const gchar *g = groups[i];
            if(Fm::FilePath::fromPathStr(g).isNative() && !QDir(QString::fromUtf8(g)).exists()) {
                g_key_file_remove_group(kf, g, nullptr);
                removed = true;
            }
        }
        g_strfreev(groups);
        if(removed) {
            g_key_file_save_to_file(kf, perFolderConfig.constData(), nullptr);
        }
    }
    g_key_file_free(kf);
}

/*bool Application::eventFilter(QObject* watched, QEvent* event) {
    if(watched == desktop()) {
        if(event->type() == QEvent::StyleChange ||
                event->type() == QEvent::ThemeChange) {
            setStyle(new ProxyStyle());
        }
    }
    return QObject::eventFilter(watched, event);
}*/

void Application::onLastWindowClosed() {

}

void Application::onSaveStateRequest(QSessionManager& /*manager*/) {

}

void Application::desktopManager(bool enabled) {
    // TODO: turn on or turn off desktpo management (desktop icons & wallpaper)
    //qDebug("desktopManager: %d", enabled);
    if(enabled) {
        if(!enableDesktopManager_) {
            // installNativeEventFilter(this);
            const auto allScreens = screens();
            for(QScreen* screen : allScreens) {
                connect(screen, &QScreen::virtualGeometryChanged, this, &Application::onVirtualGeometryChanged);
                connect(screen, &QScreen::availableGeometryChanged, this, &Application::onAvailableGeometryChanged);
                connect(screen, &QObject::destroyed, this, &Application::onScreenDestroyed);
            }
            connect(this, &QApplication::screenAdded, this, &Application::onScreenAdded);
            connect(this, &QApplication::screenRemoved, this, &Application::onScreenRemoved);

            // NOTE: there are two modes
            // When virtual desktop is used (all screens are combined to form a large virtual desktop),
            // we only create one DesktopWindow. Otherwise, we create one for each screen.
            // Under Wayland, separate desktops are created for avoiding problems.
            if(!underWayland_ && primaryScreen() && primaryScreen()->virtualSiblings().size() > 1) {
                DesktopWindow* window = createDesktopWindow(-1);
                desktopWindows_.push_back(window);
            }
            else {
                int n = qMax(allScreens.size(), 1);
                desktopWindows_.reserve(n);
                for(int i = 0; i < n; ++i) {
                    DesktopWindow* window = createDesktopWindow(i);
                    desktopWindows_.push_back(window);
                }
            }
        }
    }
    else {
        if(enableDesktopManager_) {
            int n = desktopWindows_.size();
            for(int i = 0; i < n; ++i) {
                DesktopWindow* window = desktopWindows_.at(i);
                delete window;
            }
            desktopWindows_.clear();
            const auto allScreens = screens();
            for(QScreen* screen : allScreens) {
                disconnect(screen, &QScreen::virtualGeometryChanged, this, &Application::onVirtualGeometryChanged);
                disconnect(screen, &QScreen::availableGeometryChanged, this, &Application::onAvailableGeometryChanged);
                disconnect(screen, &QObject::destroyed, this, &Application::onScreenDestroyed);
            }
            disconnect(this, &QApplication::screenAdded, this, &Application::onScreenAdded);
            disconnect(this, &QApplication::screenRemoved, this, &Application::onScreenRemoved);
            // removeNativeEventFilter(this);
        }
    }
    enableDesktopManager_ = enabled;
}

void Application::desktopPrefrences(const QString& page) {
    // show desktop preference window
    if(!desktopPreferencesDialog_) {
        desktopPreferencesDialog_ = new DesktopPreferencesDialog();

        // Should be used only one time
        desktopPreferencesDialog_->setEditDesktopFolder(!lxqtRunning_);
    }
    desktopPreferencesDialog_.data()->selectPage(page);
    desktopPreferencesDialog_.data()->show();
    desktopPreferencesDialog_.data()->raise();
    desktopPreferencesDialog_.data()->activateWindow();
}

void Application::onFindFileAccepted() {
    Fm::FileSearchDialog* dlg = static_cast<Fm::FileSearchDialog*>(sender());
    // get search settings
    settings_.setSearchNameCaseInsensitive(dlg->nameCaseInsensitive());
    settings_.setsearchContentCaseInsensitive(dlg->contentCaseInsensitive());
    settings_.setSearchNameRegexp(dlg->nameRegexp());
    settings_.setSearchContentRegexp(dlg->contentRegexp());
    settings_.setSearchRecursive(dlg->recursive());
    settings_.setSearchhHidden(dlg->searchhHidden());
    settings_.addNamePattern(dlg->namePattern());
    settings_.addContentPattern(dlg->contentPattern());

    Fm::FilePathList paths;
    paths.emplace_back(dlg->searchUri());
    MainWindow* window = MainWindow::lastActive();
    Launcher(window).launchPaths(nullptr, paths);
}

void Application::onConnectToServerAccepted() {
    ConnectServerDialog* dlg = static_cast<ConnectServerDialog*>(sender());
    QString uri = dlg->uriText();
    Fm::FilePathList paths;
    paths.push_back(Fm::FilePath::fromUri(uri.toUtf8().constData()));
    MainWindow* window = MainWindow::lastActive();
    Launcher(window).launchPaths(nullptr, paths);
}

void Application::findFiles(QStringList paths) {
    // launch file searching utility.
    Fm::FileSearchDialog* dlg = new Fm::FileSearchDialog(paths);
    connect(dlg, &QDialog::accepted, this, &Application::onFindFileAccepted);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    // set search settings
    dlg->setNameCaseInsensitive(settings_.searchNameCaseInsensitive());
    dlg->setContentCaseInsensitive(settings_.searchContentCaseInsensitive());
    dlg->setNameRegexp(settings_.searchNameRegexp());
    dlg->setContentRegexp(settings_.searchContentRegexp());
    dlg->setRecursive(settings_.searchRecursive());
    dlg->setSearchhHidden(settings_.searchhHidden());
    dlg->addNamePatterns(settings_.namePatterns());
    dlg->addContentPatterns(settings_.contentPatterns());

    dlg->show();
}

void Application::connectToServer() {
    ConnectServerDialog* dlg = new ConnectServerDialog();
    connect(dlg, &QDialog::accepted, this, &Application::onConnectToServerAccepted);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void Application::launchFiles(const QString& cwd, const QStringList& paths, bool inNewWindow, bool reopenLastTabs) {
    Fm::FilePathList pathList;
    Fm::FilePath cwd_path;
    auto _paths = paths;

    openingLastTabs_ = reopenLastTabs && settings_.reopenLastTabs() && !settings_.tabPaths().isEmpty();
    if(openingLastTabs_) {
        _paths = settings_.tabPaths();
        // forget tab paths with next windows until the last one is closed
        settings_.setTabPaths(QStringList());
    }

    for(const QString& it : std::as_const(_paths)) {
        QByteArray pathName = it.toLocal8Bit();
        Fm::FilePath path;
        if(pathName == "~") { // special case for home dir
            path = Fm::FilePath::homeDir();
        }
        if(pathName[0] == '/') { // absolute path
            path = Fm::FilePath::fromLocalPath(pathName.constData());
        }
        else if(pathName.contains(":/")) { // URI
            path = Fm::FilePath::fromUri(pathName.constData());
        }
        else { // basename
            if(Q_UNLIKELY(!cwd_path)) {
                cwd_path = Fm::FilePath::fromLocalPath(cwd.toLocal8Bit().constData());
            }
            path = cwd_path.relativePath(pathName.constData());
        }
        pathList.push_back(std::move(path));
    }

    if(!inNewWindow && settings_.singleWindowMode()) {
        MainWindow* window = MainWindow::lastActive();
        // if there is no last active window, find the last created window
        if(window == nullptr) {
            QWidgetList windows = topLevelWidgets();
            for(int i = 0; i < windows.size(); ++i) {
                auto win = windows.at(windows.size() - 1 - i);
                if(win->inherits("PCManFM::MainWindow")) {
                    window = static_cast<MainWindow*>(win);
                    break;
                }
            }
        }
        if(window != nullptr && openingLastTabs_) {
            // other folders have been opened explicitly in this window;
            // restoring of tab split number does not make sense
            settings_.setSplitViewTabsNum(0);
        }
        auto launcher = Launcher(window);
        launcher.openInNewTab();
        launcher.launchPaths(nullptr, pathList);
    }
    else {
        Launcher(nullptr).launchPaths(nullptr, pathList);
    }

    if(openingLastTabs_) {
        openingLastTabs_ = false;

        // if none of the last tabs can be opened and there is no main window yet,
        // open the current directory
        bool hasWindow = false;
        const QWidgetList windows = topLevelWidgets();
        for(const auto& win : windows) {
            if(win->inherits("PCManFM::MainWindow")) {
                hasWindow = true;
                break;
            }
        }
        if(!hasWindow) {
            _paths.clear();
            _paths.push_back(QDir::currentPath());
            launchFiles(QDir::currentPath(), _paths, inNewWindow, false);
        }
    }
}

void Application::openFolders(Fm::FileInfoList files) {
    Launcher(nullptr).launchFiles(nullptr, std::move(files));
}

void Application::openFolderInTerminal(Fm::FilePath path) {
    if(!settings_.terminal().isEmpty()) {
        Fm::GErrorPtr err;
        auto terminalName = settings_.terminal().toUtf8();
        if(!Fm::launchTerminal(terminalName.constData(), path, err)) {
            QMessageBox::critical(nullptr, tr("Error"), err.message());
        }
    }
    else {
        // show an error message and ask the user to set the command
        QMessageBox::critical(nullptr, tr("Error"), tr("Terminal emulator is not set."));
        preferences(QStringLiteral("advanced"));
    }
}

void Application::preferences(const QString& page) {
    // open preference dialog
    if(!preferencesDialog_) {
        preferencesDialog_ = new PreferencesDialog(page);
    }
    else {
        preferencesDialog_.data()->selectPage(page);
    }
    preferencesDialog_.data()->show();
    preferencesDialog_.data()->raise();
    preferencesDialog_.data()->activateWindow();
}

void Application::setWallpaper(const QString& path, const QString& modeString) {
    bool changed = false;

    if(!path.isEmpty() && path != settings_.wallpaper()) {
        if(QFile(path).exists()) {
            settings_.setWallpaper(path);
            changed = true;
        }
    }

    DesktopWindow::WallpaperMode mode;
    if(modeString.isEmpty()) {
        mode = settings_.wallpaperMode();
    }
    else {
        mode = DesktopWindow::WallpaperMode(Settings::wallpaperModeFromString(modeString));
        if(mode != settings_.wallpaperMode()) {
            changed = true;
        }
    }

    // FIXME: support different wallpapers on different screen.
    // update wallpaper
    if(changed) {
        if(enableDesktopManager_) {
            for(DesktopWindow* desktopWin :  std::as_const(desktopWindows_)) {
                if(!path.isEmpty()) {
                    desktopWin->setWallpaperFile(path);
                }
                if(mode != settings_.wallpaperMode()) {
                    settings_.setWallpaperMode(mode);
                    desktopWin->setWallpaperMode(mode);
                }
                desktopWin->updateWallpaper();
                desktopWin->update();
            }
            settings_.save(); // save the settings to the config file
        }
    }
}

/* This method receives a list of file:// URIs from DBus and for each URI opens
 * a tab showing its content.
 */
void Application::ShowFolders(const QStringList& uriList, const QString& startupId __attribute__((unused))) {
    if(!uriList.isEmpty()) {
        launchFiles(QDir::currentPath(), uriList, false, false);
    }
}

/* This method receives a list of file:// URIs from DBus and opens windows
 * or tabs for each folder, highlighting all listed items within each.
 */
void Application::ShowItems(const QStringList& uriList, const QString& startupId __attribute__((unused))) {
    std::unordered_map<Fm::FilePath, Fm::FilePathList, Fm::FilePathHash> groups;
    Fm::FilePathList folders; // used only for keeping the original order
    for(const auto& u : uriList) {
        if(auto path = Fm::FilePath::fromPathStr(u.toStdString().c_str())) {
            if(auto parent = path.parent()) {
                auto paths = groups[parent];
                if(std::find(paths.cbegin(), paths.cend(), path) == paths.cend()) {
                    groups[parent].push_back(std::move(path));
                }
                // also remember the order of parent folders
                if(std::find(folders.cbegin(), folders.cend(), parent) == folders.cend()) {
                    folders.push_back(std::move(parent));
                }
            }
        }
    }

    if(groups.empty()) {
        return;
    }

    PCManFM::MainWindow* window = nullptr;
    if(settings_.singleWindowMode()) {
        window = MainWindow::lastActive();
        if(window == nullptr) {
            QWidgetList windows = topLevelWidgets();
            for(int i = 0; i < windows.size(); ++i) {
                auto win = windows.at(windows.size() - 1 - i);
                if(win->inherits("PCManFM::MainWindow")) {
                    window = static_cast<MainWindow*>(win);
                    break;
                }
            }
        }
    }
    if(window == nullptr) {
        window = new MainWindow();
    }

    for(const auto& folder : folders) {
        window->openFolderAndSelectFles(groups[folder]);
    }

    window->show();
    window->raise();
    window->activateWindow();
}

/* This method receives a list of file:// URIs from DBus and
 * for each valid URI opens a property dialog showing its information
 */
void Application::ShowItemProperties(const QStringList& uriList, const QString& startupId __attribute__((unused))) {
    // FIXME: Should we add "Fm::FilePropsDialog::showForPath()" to libfm-qt, instead of doing this?
    Fm::FilePathList paths;
    for(const auto& u : uriList) {
        Fm::FilePath path = Fm::FilePath::fromPathStr(u.toStdString().c_str());
        if(path) {
            paths.push_back(std::move(path));
        }
    }
    if(paths.empty()) {
        return;
    }
    auto job = new Fm::FileInfoJob{std::move(paths)};
    job->setAutoDelete(true);
    connect(job, &Fm::FileInfoJob::finished, this, &Application::onPropJobFinished, Qt::BlockingQueuedConnection);
    job->runAsync();
}

void Application::onPropJobFinished() {
    auto job = static_cast<Fm::FileInfoJob*>(sender());
    for(auto file: job->files()) {
        auto dialog = Fm::FilePropsDialog::showForFile(std::move(file));
        dialog->raise();
        dialog->activateWindow();
    }
}

DesktopWindow* Application::createDesktopWindow(int screenNum) {
    DesktopWindow* window = new DesktopWindow(screenNum);

    if(screenNum == -1) { // one large virtual desktop only
        QRect rect = primaryScreen()->virtualGeometry();
        window->setGeometry(rect);
    }
    else {
        QRect rect;
        if(auto screen = window->getDesktopScreen()) {
            rect = screen->geometry();
        }
        window->setGeometry(rect);
    }

    window->updateFromSettings(settings_);
    window->show();
    window->queueRelayout(); // for some reason, sometimes needed with Qt6
    return window;
}

// called when Settings is changed to update UI
void Application::updateFromSettings() {
    // if(iconTheme.isEmpty())
    //  Fm::IconTheme::setThemeName(settings_.fallbackIconThemeName());

    // update main windows and desktop windows
    QWidgetList windows = this->topLevelWidgets();
    QWidgetList::iterator it;
    for(it = windows.begin(); it != windows.end(); ++it) {
        QWidget* window = *it;
        if(window->inherits("PCManFM::MainWindow")) {
            MainWindow* mainWindow = static_cast<MainWindow*>(window);
            mainWindow->updateFromSettings(settings_);
        }
    }
    if(desktopManagerEnabled()) {
        updateDesktopsFromSettings();
    }
}

void Application::updateDesktopsFromSettings(bool changeSlide) {
    QVector<DesktopWindow*>::iterator it;
    for(it = desktopWindows_.begin(); it != desktopWindows_.end(); ++it) {
        DesktopWindow* desktopWin = static_cast<DesktopWindow*>(*it);
        desktopWin->updateFromSettings(settings_, changeSlide);
    }
}

void Application::editBookmarks() {
    if(!editBookmarksialog_) {
        editBookmarksialog_ = new Fm::EditBookmarksDialog(Fm::Bookmarks::globalInstance());
    }
    editBookmarksialog_.data()->show();
}

void Application::initVolumeManager() {

    g_signal_connect(volumeMonitor_, "volume-added", G_CALLBACK(onVolumeAdded), this);

    if(settings_.mountOnStartup()) {
        /* try to automount all volumes */
        GList* vols = g_volume_monitor_get_volumes(volumeMonitor_);
        for(GList* l = vols; l; l = l->next) {
            GVolume* volume = G_VOLUME(l->data);
            if(g_volume_should_automount(volume)) {
                autoMountVolume(volume, false);
            }
            g_object_unref(volume);
        }
        g_list_free(vols);
    }
}

bool Application::autoMountVolume(GVolume* volume, bool interactive) {
    if(!g_volume_should_automount(volume) || !g_volume_can_mount(volume)) {
        return FALSE;
    }

    GMount* mount = g_volume_get_mount(volume);
    if(!mount) { // not mounted, automount is needed
        // try automount
        Fm::MountOperation* op = new Fm::MountOperation(interactive);
        op->mount(volume);
        if(!op->wait()) {
            return false;
        }
        if(!interactive) {
            return true;
        }
        mount = g_volume_get_mount(volume);
    }

    if(mount) {
        if(interactive && settings_.autoRun()) { // show autorun dialog
            AutoRunDialog* dlg = new AutoRunDialog(volume, mount);
            dlg->show();
        }
        g_object_unref(mount);
    }
    return true;
}

// static
void Application::onVolumeAdded(GVolumeMonitor* /*monitor*/, GVolume* volume, Application* pThis) {
    if(pThis->settings_.mountRemovable()) {
        pThis->autoMountVolume(volume, true);
    }
}

#if 0
bool Application::nativeEventFilter(const QByteArray& eventType, void* message, long* result) {
    if(eventType == "xcb_generic_event_t") { // XCB event
        // filter all native X11 events (xcb)
        xcb_generic_event_t* generic_event = reinterpret_cast<xcb_generic_event_t*>(message);
        // qDebug("XCB event: %d", generic_event->response_type & ~0x80);
        Q_FOREACH(DesktopWindow* window, desktopWindows_) {
        }
    }
    return false;
}
#endif

void Application::onScreenAdded(QScreen* newScreen) {
    if(enableDesktopManager_) {
        connect(newScreen, &QScreen::virtualGeometryChanged, this, &Application::onVirtualGeometryChanged);
        connect(newScreen, &QScreen::availableGeometryChanged, this, &Application::onAvailableGeometryChanged);
        connect(newScreen, &QObject::destroyed, this, &Application::onScreenDestroyed);
        const auto siblings = primaryScreen()->virtualSiblings();
        if(!underWayland_ // Under Wayland, separate desktops are created for avoiding problems.
           && siblings.contains(newScreen)) { // the primary screen is changed
            if(desktopWindows_.size() == 1) {
                desktopWindows_.at(0)->setGeometry(newScreen->virtualGeometry());
                if(siblings.size() > 1) { // a virtual desktop is created
                    desktopWindows_.at(0)->setScreenNum(-1);
                }
            }
            else if(desktopWindows_.isEmpty()) { // for the sake of certainty
                DesktopWindow* window = createDesktopWindow(desktopWindows_.size());
                desktopWindows_.push_back(window);
            }
        }
        else { // a separate screen is added
            DesktopWindow* window = createDesktopWindow(desktopWindows_.size());
            desktopWindows_.push_back(window);
        }
    }
}

void Application::onScreenRemoved(QScreen* oldScreen) {
    if(enableDesktopManager_){
        disconnect(oldScreen, &QScreen::virtualGeometryChanged, this, &Application::onVirtualGeometryChanged);
        disconnect(oldScreen, &QScreen::availableGeometryChanged, this, &Application::onAvailableGeometryChanged);
        disconnect(oldScreen, &QObject::destroyed, this, &Application::onScreenDestroyed);
        if(desktopWindows_.isEmpty()) {
            return;
        }
        if(desktopWindows_.size() == 1) { // a single desktop is changed
            if(primaryScreen() != nullptr) {
                desktopWindows_.at(0)->setGeometry(primaryScreen()->virtualGeometry());
                if(primaryScreen()->virtualSiblings().size() == 1) {
                    desktopWindows_.at(0)->setScreenNum(0); // there is no virtual desktop anymore
                }
            }
            else if (screens().isEmpty()) { // for the sake of certainty
                desktopWindows_.at(0)->setScreenNum(0);
            }
        }
        else { // a separate desktop is removed
            int n = desktopWindows_.size();
            for(int i = 0; i < n; ++i) {
                DesktopWindow* window = desktopWindows_.at(i);
                if(window->getDesktopScreen() == oldScreen) {
                    desktopWindows_.remove(i);
                    delete window;
                    break;
                }
            }
        }
    }
}

void Application::onScreenDestroyed(QObject* screenObj) {
    // NOTE by PCMan: This is a workaround for Qt 5 bug #40681.
    // With this very dirty workaround, we can fix lxqt/lxqt bug #204, #205, and #206.
    // Qt 5 has two new regression bugs which breaks lxqt-panel in a multihead environment.
    // #40681: Regression bug: QWidget::winId() returns old value and QEvent::WinIdChange event is not emitted sometimes. (multihead setup)
    // #40791: Regression: QPlatformWindow, QWindow, and QWidget::winId() are out of sync.
    // Explanations for the workaround:
    // Internally, Qt maintains a list of QScreens and update it when XRandR configuration changes.
    // When the user turn off an monitor with xrandr --output <xxx> --off, this will destroy the QScreen
    // object which represent the output. If the QScreen being destroyed contains our panel widget,
    // Qt will call QWindow::setScreen(0) on the internal windowHandle() of our panel widget to move it
    // to the primary screen. However, moving a window to a different screen is more than just changing
    // its position. With XRandR, all screens are actually part of the same virtual desktop. However,
    // this is not the case in other setups, such as Xinerama and moving a window to another screen is
    // not possible unless you destroy the widget and create it again for a new screen.
    // Therefore, Qt destroy the widget and re-create it when moving our panel to a new screen.
    // Unfortunately, destroying the window also destroy the child windows embedded into it,
    // using XEMBED such as the tray icons. (#206)
    // Second, when the window is re-created, the winId of the QWidget is changed, but Qt failed to
    // generate QEvent::WinIdChange event so we have no way to know that. We have to set
    // some X11 window properties using the native winId() to make it a dock, but this stop working
    // because we cannot get the correct winId(), so this causes #204 and #205.
    //
    // The workaround is very simple. Just completely destroy the window before Qt has a chance to do
    // QWindow::setScreen() for it. Later, we recreate the window ourselves. So this can bypassing the Qt bugs.
    if(enableDesktopManager_) {
        bool reloadNeeded = false;
        // FIXME: add workarounds for Qt5 bug #40681 and #40791 here.
        for(DesktopWindow* desktopWin :  std::as_const(desktopWindows_)) {
            if(desktopWin->windowHandle()->screen() == screenObj) {
                desktopWin->destroy(); // destroy the underlying native window
                reloadNeeded = true;
            }
        }
        if(reloadNeeded) {
            QTimer::singleShot(0, this, &Application::reloadDesktopsAsNeeded);
        }
    }
}

void Application::reloadDesktopsAsNeeded() {
    if(enableDesktopManager_) {
        // workarounds for Qt5 bug #40681 and #40791 here.
        for(DesktopWindow* desktopWin : std::as_const(desktopWindows_)) {
            if(!desktopWin->windowHandle()) {
                desktopWin->create(); // re-create the underlying native window
                desktopWin->queueRelayout();
                desktopWin->show();
            }
        }
    }
}

void Application::onVirtualGeometryChanged(const QRect& /*rect*/) {
    // update desktop geometries
    if(enableDesktopManager_) {
        for(DesktopWindow* desktopWin : std::as_const(desktopWindows_)) {
            auto desktopScreen = desktopWin->getDesktopScreen();
            if(desktopScreen) {
                desktopWin->setGeometry(desktopScreen->virtualGeometry());
            }
        }
    }
}

void Application::onAvailableGeometryChanged(const QRect& /*rect*/) {
    // update desktop layouts
    if(enableDesktopManager_) {
        for(DesktopWindow* desktopWin : std::as_const(desktopWindows_)) {
            desktopWin->queueRelayout();
        }
    }
}


static int sigterm_fd[2];

static void sigtermHandler(int) {
    char c = 1;
    auto w = ::write(sigterm_fd[0], &c, sizeof(c));
    Q_UNUSED(w);
}

void Application::installSigtermHandler() {
    if(::socketpair(AF_UNIX, SOCK_STREAM, 0, sigterm_fd) == 0) {
        QSocketNotifier* notifier = new QSocketNotifier(sigterm_fd[1], QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated, this, &Application::onSigtermNotified);

        struct sigaction action;
        action.sa_handler = sigtermHandler;
        ::sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        if(::sigaction(SIGTERM, &action, nullptr) != 0) {
            qWarning("Couldn't install SIGTERM handler");
        }
    }
    else {
        qWarning("Couldn't create SIGTERM socketpair");
    }
}

void Application::onSigtermNotified() {
    if(QSocketNotifier* notifier = qobject_cast<QSocketNotifier*>(sender())) {
        notifier->setEnabled(false);
        char c;
        auto r = ::read(sigterm_fd[1], &c, sizeof(c));
        Q_UNUSED(r);
        // close all windows cleanly; otherwise, we might get this warning:
        // "QBasicTimer::start: QBasicTimer can only be used with threads started with QThread"
        const auto windows = topLevelWidgets();
        for(const auto& win : windows) {
            if(win->inherits("PCManFM::MainWindow")) {
                MainWindow* mainWindow = static_cast<MainWindow*>(win);
                mainWindow->close();
            }
        }
        quit();
        notifier->setEnabled(true);
    }
}

} // namespace PCManFM
