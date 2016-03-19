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
#include <QDBusInterface>
#include <QDir>
#include <QDesktopWidget>
#include <QVector>
#include <QLocale>
#include <QLibraryInfo>
#include <QPixmapCache>
#include <QFile>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QSocketNotifier>
#include <gio/gio.h>
#include <sys/socket.h>

#include "applicationadaptor.h"
#include "preferencesdialog.h"
#include "desktoppreferencesdialog.h"
#include <libfm-qt/mountoperation.h>
#include "autorundialog.h"
#include "launcher.h"
#include <libfm-qt/filesearchdialog.h>

#include <QScreen>
#include <QWindow>

#include <X11/Xlib.h>

#include "xdgdir.h"
#include <QFileSystemWatcher>

namespace PCManFM {

static const char* serviceName = "org.pcmanfm.PCManFM";
static const char* ifaceName = "org.pcmanfm.Application";

int ProxyStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const {
  Application* app = static_cast<Application*>(qApp);
  if(hint == QStyle::SH_ItemView_ActivateItemOnSingleClick)
    return app->settings().singleClick();
  return QProxyStyle::styleHint(hint, option, widget, returnData);
}

Application::Application(int& argc, char** argv):
  QApplication(argc, argv),
  libFm_(),
  settings_(),
  profileName_("default"),
  daemonMode_(false),
  enableDesktopManager_(false),
  desktopWindows_(),
  preferencesDialog_(),
  editBookmarksialog_(),
  volumeMonitor_(NULL),
  userDirsWatcher_(NULL),
  lxqtRunning_(false) {

  argc_ = argc;
  argv_ = argv;

  setApplicationVersion(PCMANFM_QT_VERSION);

  // QDBusConnection::sessionBus().registerObject("/org/pcmanfm/Application", this);
  QDBusConnection dbus = QDBusConnection::sessionBus();
  if(dbus.registerService(serviceName)) {
    // we successfully registered the service
    isPrimaryInstance = true;
    setStyle(new ProxyStyle());
    desktop()->installEventFilter(this);

    new ApplicationAdaptor(this);
    dbus.registerObject("/Application", this);

    connect(this, &Application::aboutToQuit, this, &Application::onAboutToQuit);
    // aboutToQuit() is not signalled on SIGTERM, install signal handler
    installSigtermHandler();
    settings_.load(profileName_);

    // decrease the cache size to reduce memory usage
    QPixmapCache::setCacheLimit(2048);

    if(settings_.useFallbackIconTheme()) {
      QIcon::setThemeName(settings_.fallbackIconThemeName());
      Fm::IconTheme::checkChanged();
    }

    // Check if LXQt Session is running. LXQt has it's own Desktop Folder
    // editor. We just hide our editor when LXQt is running.
    QDBusInterface* lxqtSessionIface = new QDBusInterface(
                                        QStringLiteral("org.lxqt.session"),
                                        QStringLiteral("/LXQtSession"));
    if (lxqtSessionIface) {
      if (lxqtSessionIface->isValid()) {
          lxqtRunning_ = true;
          userDesktopFolder_ = XdgDir::readDesktopDir();
          initWatch();
      }
      delete lxqtSessionIface;
      lxqtSessionIface = 0;
    }
  }
  else {
    // an service of the same name is already registered.
    // we're not the first instance
    isPrimaryInstance = false;
  }
}

Application::~Application() {
  desktop()->removeEventFilter(this);

  if(volumeMonitor_) {
    g_signal_handlers_disconnect_by_func(volumeMonitor_, gpointer(onVolumeAdded), this);
    g_object_unref(volumeMonitor_);
  }

  // if(enableDesktopManager_)
  //   removeNativeEventFilter(this);
}

void Application::initWatch()
{
  QFile file_ (QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/user-dirs.dirs"));
  if(! file_.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << Q_FUNC_INFO << "Could not read: " << userDirsFile_;
    userDirsFile_ = QString();
  } else {
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

  QCommandLineOption profileOption(QStringList() << "p" << "profile", tr("Name of configuration profile"), tr("PROFILE"));
  parser.addOption(profileOption);

  QCommandLineOption daemonOption(QStringList() << "d" << "daemon-mode", tr("Run PCManFM as a daemon"));
  parser.addOption(daemonOption);

  QCommandLineOption quitOption(QStringList() << "q" << "quit", tr("Quit PCManFM"));
  parser.addOption(quitOption);

  QCommandLineOption desktopOption("desktop", tr("Launch desktop manager"));
  parser.addOption(desktopOption);

  QCommandLineOption desktopOffOption("desktop-off", tr("Turn off desktop manager if it's running"));
  parser.addOption(desktopOffOption);

  QCommandLineOption desktopPrefOption("desktop-pref", tr("Open desktop preference dialog on the page with the specified name"), tr("NAME"));
  parser.addOption(desktopPrefOption);

  QCommandLineOption newWindowOption(QStringList() << "n" << "new-window", tr("Open new window"));
  parser.addOption(newWindowOption);

  QCommandLineOption findFilesOption(QStringList() << "f" << "find-files", tr("Open Find Files utility"));
  parser.addOption(findFilesOption);

  QCommandLineOption setWallpaperOption(QStringList() << "w" << "set-wallpaper", tr("Set desktop wallpaper from image FILE"), tr("FILE"));
  parser.addOption(setWallpaperOption);

  QCommandLineOption wallpaperModeOption("wallpaper-mode", tr("Set mode of desktop wallpaper. MODE=(%1)").arg("color|stretch|fit|center|tile"), tr("MODE"));
  parser.addOption(wallpaperModeOption);

  QCommandLineOption showPrefOption("show-pref", tr("Open Preferences dialog on the page with the specified name"), tr("NAME"));
  parser.addOption(showPrefOption);

  parser.addPositionalArgument("files", tr("Files or directories to open"), tr("[FILE1, FILE2,...]"));

  parser.process(arguments());

  if(isPrimaryInstance) {
    qDebug("isPrimaryInstance");

    if(parser.isSet(daemonOption))
      daemonMode_ = true;
    if(parser.isSet(profileOption))
      profileName_ = parser.value(profileOption);

    // load settings
    settings_.load(profileName_);

    // desktop icon management
    if(parser.isSet(desktopOption)) {
      desktopManager(true);
      keepRunning = true;
    }
    else if(parser.isSet(desktopOffOption))
      desktopManager(false);

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
    else if(parser.isSet(setWallpaperOption) || parser.isSet(wallpaperModeOption)) // set wall paper
      setWallpaper(parser.value(setWallpaperOption), parser.value(wallpaperModeOption));
    else {
      if(!parser.isSet(desktopOption) && !parser.isSet(desktopOffOption)) {
        QStringList paths = parser.positionalArguments();
        if(paths.isEmpty()) {
          // if no path is specified and we're using daemon mode,
          // don't open current working directory
          if(!daemonMode_)
            paths.push_back(QDir::currentPath());
        }
        if(!paths.isEmpty())
          launchFiles(QDir::currentPath(), paths, parser.isSet(newWindowOption));
        keepRunning = true;
      }
    }
  }
  else {
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusInterface iface(serviceName, "/Application", ifaceName, dbus, this);
    if(parser.isSet(quitOption)) {
      iface.call("quit");
      return false;
    }

    if(parser.isSet(desktopOption))
      iface.call("desktopManager", true);
    else if(parser.isSet(desktopOffOption))
      iface.call("desktopManager", false);

    if(parser.isSet(desktopPrefOption)) { // desktop preference dialog
      iface.call("desktopPrefrences", parser.value(desktopPrefOption));
    }
    else if(parser.isSet(findFilesOption)) { // file searching utility
      iface.call("findFiles", parser.positionalArguments());
    }
    else if(parser.isSet(showPrefOption)) { // preferences dialog
      iface.call("preferences", parser.value(showPrefOption));
    }
    else if(parser.isSet(setWallpaperOption) || parser.isSet(wallpaperModeOption)) { // set wall paper
      iface.call("setWallpaper", parser.value(setWallpaperOption), parser.value(wallpaperModeOption));
    }
    else {
      if(!parser.isSet(desktopOption) && !parser.isSet(desktopOffOption)) {
        QStringList paths = parser.positionalArguments();
        if(paths.isEmpty()) {
          paths.push_back(QDir::currentPath());
        }
        iface.call("launchFiles", QDir::currentPath(), paths, parser.isSet(newWindowOption));
      }
    }
  }
  return keepRunning;
}

void Application::init() {

  // install the translations built-into Qt itself
  qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  installTranslator(&qtTranslator);

  // install libfm-qt translator
  installTranslator(libFm_.translator());

  // install our own tranlations
  translator.load("pcmanfm-qt_" + QLocale::system().name(), PCMANFM_DATA_DIR "/translations");
  installTranslator(&translator);
}

int Application::exec() {

  if(!parseCommandLineArgs())
    return 0;

  if(daemonMode_) // keep running even when there is no window opened.
    setQuitOnLastWindowClosed(false);

  volumeMonitor_ = g_volume_monitor_get();
  // delay the volume manager a little because in newer versions of glib/gio there's a problem.
  // when the first volume monitor object is created, it discovers volumes asynchonously.
  // g_volume_monitor_get() immediately returns while the monitor is still discovering devices.
  // So initially g_volume_monitor_get_volumes() returns nothing, but shortly after that
  // we get volume-added signals for all of the volumes. This is not what we want.
  // So, we wait for 3 seconds here to let it finish device discovery.
  QTimer::singleShot(3000, this, SLOT(initVolumeManager()));

  return QCoreApplication::exec();
}


void Application::onUserDirsChanged()
{
  qDebug() << Q_FUNC_INFO;
  bool file_deleted = !userDirsWatcher_->files().contains(userDirsFile_);
  if(file_deleted) {
    // if our config file is already deleted, reinstall a new watcher
    userDirsWatcher_->addPath(userDirsFile_);
  }

  const QString d = XdgDir::readDesktopDir();
  if (d != userDesktopFolder_) {
    userDesktopFolder_ = d;
    const QDir dir(d);
    if (dir.exists()) {
      const int N = desktopWindows_.size();
      for(int i = 0; i < N; ++i) {
        desktopWindows_.at(i)->setDesktopFolder();
      }
    } else {
        qWarning("Application::onUserDirsChanged: %s doesn't exist",
                    userDesktopFolder_.toUtf8().constData());
    }
  }
}

void Application::onAboutToQuit() {
  qDebug("aboutToQuit");
  settings_.save();
}

bool Application::eventFilter(QObject* watched, QEvent* event) {
  if(watched == desktop()) {
    switch(event->type()) {
      case QEvent::StyleChange:
      case QEvent::ThemeChange:
        setStyle(new ProxyStyle());
    };
  }
  return QObject::eventFilter(watched, event);
}

void Application::onLastWindowClosed() {

}

void Application::onSaveStateRequest(QSessionManager& manager) {

}

void Application::desktopManager(bool enabled) {
  // TODO: turn on or turn off desktpo management (desktop icons & wallpaper)
  qDebug("desktopManager: %d", enabled);
  QDesktopWidget* desktopWidget = desktop();
  if(enabled) {
    if(!enableDesktopManager_) {
      // installNativeEventFilter(this);
      Q_FOREACH(QScreen* screen, screens()) {
        connect(screen, &QScreen::virtualGeometryChanged, this, &Application::onVirtualGeometryChanged);
        connect(screen, &QObject::destroyed, this, &Application::onScreenDestroyed);
      }
      connect(this, &QApplication::screenAdded, this, &Application::onScreenAdded);
      connect(desktopWidget, &QDesktopWidget::resized, this, &Application::onScreenResized);
      connect(desktopWidget, &QDesktopWidget::screenCountChanged, this, &Application::onScreenCountChanged);

      // NOTE: there are two modes
      // When virtual desktop is used (all screens are combined to form a large virtual desktop),
      // we only create one DesktopWindow. Otherwise, we create one for each screen.
      if(desktopWidget->isVirtualDesktop()) {
        DesktopWindow* window = createDesktopWindow(-1);
        desktopWindows_.push_back(window);
      }
      else {
        int n = desktopWidget->numScreens();
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
      disconnect(desktopWidget, &QDesktopWidget::resized, this, &Application::onScreenResized);
      disconnect(desktopWidget, &QDesktopWidget::screenCountChanged, this, &Application::onScreenCountChanged);
      int n = desktopWindows_.size();
      for(int i = 0; i < n; ++i) {
        DesktopWindow* window = desktopWindows_.at(i);
        delete window;
      }
      desktopWindows_.clear();
      Q_FOREACH(QScreen* screen, screens()) {
        disconnect(screen, &QScreen::virtualGeometryChanged, this, &Application::onVirtualGeometryChanged);
        disconnect(screen, &QObject::destroyed, this, &Application::onScreenDestroyed);
      }
      disconnect(this, &QApplication::screenAdded, this, &Application::onScreenAdded);
      // removeNativeEventFilter(this);
    }
  }
  enableDesktopManager_ = enabled;
}

void Application::desktopPrefrences(QString page) {
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
  Fm::Path uri = dlg->searchUri();
  // FIXME: we should be able to open it in an existing window
  FmPathList* paths = fm_path_list_new();
  fm_path_list_push_tail(paths, uri.data());
  Launcher(NULL).launchPaths(NULL, paths);
  fm_path_list_unref(paths);
}

void Application::findFiles(QStringList paths) {
  // launch file searching utility.
  Fm::FileSearchDialog* dlg = new Fm::FileSearchDialog(paths);
  connect(dlg, &QDialog::accepted, this, &Application::onFindFileAccepted);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
}

void Application::launchFiles(QString cwd, QStringList paths, bool inNewWindow) {
  FmPathList* pathList = fm_path_list_new();
  FmPath* cwd_path = NULL;
  QStringList::iterator it;
  Q_FOREACH(const QString& it, paths) {
    QByteArray pathName = it.toLocal8Bit();
    FmPath* path = NULL;
    if(pathName[0] == '/') // absolute path
        path = fm_path_new_for_path(pathName.constData());
    else if(pathName.contains(":/")) // URI
        path = fm_path_new_for_uri(pathName.constData());
    else if(pathName == "~") // special case for home dir
        path = fm_path_ref(fm_path_get_home());
    else // basename
    {
        if(Q_UNLIKELY(!cwd_path))
            cwd_path = fm_path_new_for_str(cwd.toLocal8Bit().constData());
        path = fm_path_new_relative(cwd_path, pathName.constData());
    }
    fm_path_list_push_tail(pathList, path);
    fm_path_unref(path);
  }
  if(cwd_path)
      fm_path_unref(cwd_path);

  Launcher(NULL).launchPaths(NULL, pathList);
  fm_path_list_unref(pathList);
}

void Application::openFolders(FmFileInfoList* files) {
  Launcher(NULL).launchFiles(NULL, files);
}

void Application::openFolderInTerminal(FmPath* path) {
  if(!settings_.terminal().isEmpty()) {
    char* cwd_str;
    if(fm_path_is_native(path))
      cwd_str = fm_path_to_str(path);
    else { // gio will map remote filesystems to local FUSE-mounted paths here.
      GFile* gf = fm_path_to_gfile(path);
      cwd_str = g_file_get_path(gf);
      g_object_unref(gf);
    }
    GError* err = NULL;
    if(!fm_terminal_launch(cwd_str, &err)) {
      QMessageBox::critical(NULL, tr("Error"), QString::fromUtf8(err->message));
      g_error_free(err);
    }
    g_free(cwd_str);
  }
  else {
    // show an error message and ask the user to set the command
    QMessageBox::critical(NULL, tr("Error"), tr("Terminal emulator is not set."));
    preferences("advanced");
  }
}

void Application::preferences(QString page) {
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

void Application::setWallpaper(QString path, QString modeString) {
  static const char* valid_wallpaper_modes[] = {"color", "stretch", "fit", "center", "tile"};
  DesktopWindow::WallpaperMode mode = settings_.wallpaperMode();
  bool changed = false;

  if(!path.isEmpty() && path != settings_.wallpaper()) {
    if(QFile(path).exists()) {
      settings_.setWallpaper(path);
      changed = true;
    }
  }
  // convert mode string to value
  for(int i = 0; i < G_N_ELEMENTS(valid_wallpaper_modes); ++i) {
    if(modeString == valid_wallpaper_modes[i]) {
      mode = (DesktopWindow::WallpaperMode)i;
      if(mode != settings_.wallpaperMode())
        changed = true;
      break;
    }
  }
  // FIXME: support different wallpapers on different screen.
  // update wallpaper
  if(changed) {
    if(enableDesktopManager_) {
      Q_FOREACH(DesktopWindow * desktopWindow, desktopWindows_) {
        if(!path.isEmpty())
          desktopWindow->setWallpaperFile(path);
        if(mode != settings_.wallpaperMode())
          desktopWindow->setWallpaperMode(mode);
        desktopWindow->updateWallpaper();
      }
      settings_.save(); // save the settings to the config file
    }
  }
}

void Application::onScreenResized(int num) {
  if(desktop()->isVirtualDesktop()) {
    // in virtual desktop mode, we only have one desktop window. that is the first one.
    DesktopWindow* window = desktopWindows_.at(0);
    window->setGeometry(desktop()->geometry());
  }
  else {
    DesktopWindow* window = desktopWindows_.at(num);
    QRect rect = desktop()->screenGeometry(num);
    window->setGeometry(rect);
  }
}

DesktopWindow* Application::createDesktopWindow(int screenNum) {
  DesktopWindow* window = new DesktopWindow(screenNum);
  if(screenNum == -1) { // one large virtual desktop only
    QRect rect = desktop()->geometry();
    window->setGeometry(rect);
  }
  else {
    QRect rect = desktop()->screenGeometry(screenNum);
    window->setGeometry(rect);
  }
  window->updateFromSettings(settings_);
  window->show();
  return window;
}

void Application::onScreenCountChanged(int newCount) {
  QDesktopWidget* desktopWidget = desktop();
  bool oldVirtual = (desktopWindows_.size() == 1 && desktopWindows_.at(0)->screenNum() == -1);
  bool isVirtual = desktopWidget->isVirtualDesktop();

  if(oldVirtual && isVirtual) {
    // if we are using virtual desktop mode previously, and the new mode is sitll virtual
    // no further change is needed, only do relayout.
    desktopWindows_.at(0)->queueRelayout();
    return;
  }

  // we used non-virtual mode originally, but now we're switched to virtual mode
  if(isVirtual)
      newCount = 1; // we only want one desktop window for all screens in virtual mode

  if(newCount > desktopWindows_.size()) {
    // add more desktop windows
    for(int i = desktopWindows_.size(); i < newCount; ++i) {
      DesktopWindow* desktop = createDesktopWindow(i);
      desktopWindows_.push_back(desktop);
    }
  }
  else if(newCount < desktopWindows_.size()) {
    // delete excessive desktop windows
    for(int i = newCount; i < desktopWindows_.size(); ++i) {
      DesktopWindow* desktop = desktopWindows_.at(i);
      delete desktop;
    }
    desktopWindows_.resize(newCount);
  }

  if(newCount == 1) { // now only 1 screen is in use
    DesktopWindow* desktop = desktopWindows_.at(0);
    if(isVirtual)
      desktop->setScreenNum(-1);
    else // non-virtual mode, and we only have 1 screen
      desktop->setScreenNum(0);
    desktop->updateWallpaper();
  }
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
  if(desktopManagerEnabled())
    updateDesktopsFromSettings();
}

void Application::updateDesktopsFromSettings() {
  QVector<DesktopWindow*>::iterator it;
  for(it = desktopWindows_.begin(); it != desktopWindows_.end(); ++it) {
    DesktopWindow* desktopWindow = static_cast<DesktopWindow*>(*it);
    desktopWindow->updateFromSettings(settings_);
  }
}

void Application::editBookmarks() {
  if(!editBookmarksialog_) {
    FmBookmarks* bookmarks = fm_bookmarks_dup();
    editBookmarksialog_ = new Fm::EditBookmarksDialog(bookmarks);
    g_object_unref(bookmarks);
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
      if(g_volume_should_automount(volume))
        autoMountVolume(volume, false);
      g_object_unref(volume);
    }
    g_list_free(vols);
  }
}

bool Application::autoMountVolume(GVolume* volume, bool interactive) {
  if(!g_volume_should_automount(volume) || !g_volume_can_mount(volume))
    return FALSE;

  GMount* mount = g_volume_get_mount(volume);
  if(!mount) { // not mounted, automount is needed
    // try automount
    Fm::MountOperation* op = new Fm::MountOperation(interactive);
    op->mount(volume);
    if(!op->wait())
      return false;
    if(!interactive)
      return true;
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
void Application::onVolumeAdded(GVolumeMonitor* monitor, GVolume* volume, Application* pThis) {
  if(pThis->settings_.mountRemovable())
    pThis->autoMountVolume(volume, true);
}

#if 0
bool Application::nativeEventFilter(const QByteArray & eventType, void * message, long * result) {
  if(eventType == "xcb_generic_event_t") { // XCB event
    // filter all native X11 events (xcb)
    xcb_generic_event_t* generic_event = reinterpret_cast<xcb_generic_event_t*>(message);
    // qDebug("XCB event: %d", generic_event->response_type & ~0x80);
    Q_FOREACH(DesktopWindow * window, desktopWindows_) {
    }
  }
  return false;
}
#endif

void Application::onScreenAdded(QScreen* newScreen) {
  if(enableDesktopManager_) {
    connect(newScreen, &QScreen::virtualGeometryChanged, this, &Application::onVirtualGeometryChanged);
    connect(newScreen, &QObject::destroyed, this, &Application::onScreenDestroyed);
  }
}

void Application::onScreenDestroyed(QObject* screenObj) {
  // NOTE by PCMan: This is a workaround for Qt 5 bug #40681.
  // With this very dirty workaround, we can fix lxde/lxde-qt bug #204, #205, and #206.
  // Qt 5 has two new regression bugs which breaks lxqt-panel in a multihead environment.
  // #40681: Regression bug: QWidget::winId() returns old value and QEvent::WinIdChange event is not emitted sometimes. (multihead setup)
  // #40791: Regression: QPlatformWindow, QWindow, and QWidget::winId() are out of sync.
  // Explanations for the workaround:
  // Internally, Qt mantains a list of QScreens and update it when XRandR configuration changes.
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
    Q_FOREACH(DesktopWindow* desktop, desktopWindows_) {
      if(desktop->windowHandle()->screen() == screenObj) {
        desktop->destroy(); // destroy the underlying native window
        reloadNeeded = true;
      }
    }
    if(reloadNeeded)
        QTimer::singleShot(0, this, SLOT(reloadDesktopsAsNeeded()));
  }
}

void Application::reloadDesktopsAsNeeded() {
  if(enableDesktopManager_) {
    // workarounds for Qt5 bug #40681 and #40791 here.
    Q_FOREACH(DesktopWindow* desktop, desktopWindows_) {
      if(!desktop->windowHandle()) {
        desktop->create(); // re-create the underlying native window
        desktop->queueRelayout();
        desktop->show();
      }
    }
  }
}

// This slot is for Qt 5 onlt, but the stupid Qt moc cannot do conditional compilation
// so we have to define it for Qt 4 as well.
void Application::onVirtualGeometryChanged(const QRect& rect) {
  // NOTE: the following is a workaround for Qt bug 32567.
  // https://bugreports.qt-project.org/browse/QTBUG-32567
  // Though the status of the bug report is closed, it's not yet fixed for X11.
  // In theory, QDesktopWidget should emit "workAreaResized()" signal when the work area
  // of any screen is changed, but in fact it does not do it.
  // However, QScreen provided since Qt5 does not have the bug and
  // virtualGeometryChanged() is emitted correctly when the workAreas changed.
  // So we use it in Qt5.
  if(enableDesktopManager_) {
    // qDebug() << "onVirtualGeometryChanged";
    Q_FOREACH(DesktopWindow* desktop, desktopWindows_) {
      desktop->queueRelayout();
    }
  }
}


static int sigterm_fd[2];

static void sigtermHandler(int) {
  char c = 1;
  ::write(sigterm_fd[0], &c, sizeof(c));
}

void Application::installSigtermHandler() {
  if(::socketpair(AF_UNIX, SOCK_STREAM, 0, sigterm_fd) == 0) {
    QSocketNotifier* notifier = new QSocketNotifier(sigterm_fd[1], QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &Application::onSigtermNotified);

    struct sigaction action;
    action.sa_handler = sigtermHandler;
    ::sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if(::sigaction(SIGTERM, &action, 0) != 0) {
      qWarning("Couldn't install SIGTERM handler");
    }
  } else {
    qWarning("Couldn't create SIGTERM socketpair");
  }
}

void Application::onSigtermNotified() {
  if (QSocketNotifier* notifier = qobject_cast<QSocketNotifier*>(sender())) {
    notifier->setEnabled(false);
    char c;
    ::read(sigterm_fd[1], &c, sizeof(c));
    quit();
    notifier->setEnabled(true);
  }
}

} // namespace PCManFM
