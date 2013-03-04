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


#include "application.h"
#include "mainwindow.h"
#include "desktopwindow.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDir>
#include <QDesktopWidget>
#include <QVector>

#include "applicationadaptor.h"
#include "preferencesdialog.h"

using namespace PCManFM;
static const char* serviceName = "org.pcmanfm.PCManFM";
static const char* ifaceName = "org.pcmanfm.Application";

Application::Application(int& argc, char** argv):
  QApplication(argc, argv),
  fmApp_(),
  settings_(),
  profileName("default"),
  daemonMode_(false),
  desktopWindows_(),
  enableDesktopManager_(false),
  preferencesDialog_() {

  // QDBusConnection::sessionBus().registerObject("/org/pcmanfm/Application", this);
  QDBusConnection dbus = QDBusConnection::sessionBus();
  if(dbus.registerService(serviceName)) {
    // we successfully registered the service
    isPrimaryInstance = true;

    new ApplicationAdaptor(this);
    dbus.registerObject("/Application", this);

    connect(this, SIGNAL(aboutToQuit()), SLOT(onAboutToQuit()));
    settings_.load(profileName);
    Fm::IconTheme::setThemeName(settings_.iconThemeName());
  }
  else {
    // an service of the same name is already registered.
    // we're not the first instance
    isPrimaryInstance = false;
  }
}

Application::~Application() {

}

struct FakeTr {
  FakeTr(int reserved = 20) {
    strings.reserve(reserved);
  }

  const char* operator() (const char* str) {
    QString translated = QApplication::translate(NULL, str);
    strings.push_back(translated.toUtf8());
    return strings.back().constData();
  }
  QVector<QByteArray> strings;
};

bool Application::parseCommandLineArgs(int argc, char** argv) {
  bool keepRunning = false;

  // It's really a shame that the great Qt library does not come
  // with any command line parser.
  // After trying some Qt ways, I finally realized that glib is the best.
  // Simple, efficient, effective, and does not use signal/slot!
  // The only drawback is the translated string returned by tr() is
  // a temporary one. We need to store them in a list to keep them alive. :-(

  char* profile = NULL;
  gboolean daemon_mode = FALSE;
  gboolean ask_quit = FALSE;
  gboolean desktop = FALSE;
  gboolean desktop_off = FALSE;
  char* desktop_pref = NULL;
  char* wallpaper = NULL;
  char* wallpaper_mode = NULL;
  char* show_pref = NULL;
  gboolean new_window = FALSE;
  gboolean find_files = FALSE;
  char** file_names = NULL;
  {
    FakeTr tr; // a functor used to override QObject::tr().
    // it convert the translated strings to UTF8 and add them to a list to
    // keep them alive during the option parsing process.
    GOptionEntry option_entries[] = {
      /* options only acceptable by first pcmanfm instance. These options are not passed through IPC */
      {"profile", 'p', 0, G_OPTION_ARG_STRING, &profile, tr("Name of configuration profile"), tr("PROFILE") },
      {"daemon-mode", 'd', 0, G_OPTION_ARG_NONE, &daemon_mode, tr("Run PCManFM as a daemon"), NULL },
      // options that are acceptable for every instance of pcmanfm and will be passed through IPC.
      {"quit", 'p', 0, G_OPTION_ARG_NONE, &ask_quit, tr("Quit PCManFM"), NULL},
      {"desktop", '\0', 0, G_OPTION_ARG_NONE, &desktop, tr("Launch desktop manager"), NULL },
      {"desktop-off", '\0', 0, G_OPTION_ARG_NONE, &desktop_off, tr("Turn off desktop manager if it's running"), NULL },
      {"desktop-pref", '\0', 0, G_OPTION_ARG_STRING, &desktop_pref, tr("Open desktop preference dialog on the page with the specified name"), tr("NAME") },
      {"set-wallpaper", 'w', 0, G_OPTION_ARG_FILENAME, &wallpaper, tr("Set desktop wallpaper from image FILE"), tr("FILE") },
      // don't translate list of modes in description, please
      {"wallpaper-mode", '\0', 0, G_OPTION_ARG_STRING, &wallpaper_mode, tr("Set mode of desktop wallpaper. MODE=(color|stretch|fit|center|tile)"), tr("MODE") },
      {"show-pref", '\0', 0, G_OPTION_ARG_STRING, &show_pref, tr("Open Preferences dialog on the page with the specified name"), tr("NAME") },
      {"new-window", 'n', 0, G_OPTION_ARG_NONE, &new_window, tr("Open new window"), NULL },
      {"find-files", 'f', 0, G_OPTION_ARG_NONE, &find_files, tr("Open Find Files utility"), NULL },
      {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &file_names, NULL, tr("[FILE1, FILE2,...]")},
      { NULL }
    };

    GOptionContext* context = g_option_context_new("");
    g_option_context_add_main_entries(context, option_entries, NULL);
    GError* error = NULL;
    if(!g_option_context_parse(context, &argc, &argv, &error)) {
      // show error and exit
      g_fprintf(stderr, "%s\n\n", error->message);
      g_error_free(error);
      g_option_context_free(context);
      return false;
    }
    g_option_context_free(context);
  }

  if(isPrimaryInstance) {
    qDebug("isPrimaryInstance");

    if(daemon_mode)
      daemonMode_ = true;
    if(profile)
      profileName = profile;

    // load settings
    settings_.load(profileName);

    // desktop icon management
    if(desktop) {
      desktopManager(true);
      keepRunning = true;
    }
    else if(desktop_off)
      desktopManager(false);

    if(desktop_pref) // desktop preference dialog
      desktopPrefrences(desktop_pref);
    else if(find_files) { // file searching utility
      QStringList paths;
      if(file_names) {
        for(char** filename = file_names; *filename; ++filename) {
          QString path(*filename);
          paths.push_back(path);
        }
      }
      findFiles(paths);
    }
    else if(show_pref) // preferences dialog
      preferences(show_pref);
    else if(wallpaper || wallpaper_mode) // set wall paper
      setWallpaper(wallpaper, wallpaper_mode);
    else {
      if(!desktop && !desktop_off) {
        QStringList paths;
        if(file_names) {
          for(char** filename = file_names; *filename; ++filename) {
            QString path(*filename);
            paths.push_back(path);
          }
        }
        else {
          // if no path is specified and we're using daemon mode,
          // don't open current working directory
          if(!daemonMode_)
            paths.push_back(QDir::currentPath());
        }
        if(!paths.isEmpty())
          launchFiles(paths, (bool)new_window);
        keepRunning = true;
      }
    }
  }
  else {
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusInterface iface(serviceName, "/Application", ifaceName, dbus, this);
    if(ask_quit) {
      iface.call("quit");
      return false;
    }

    if(desktop)
      iface.call("desktopManager", true);
    else if(desktop_off)
      iface.call("desktopManager", false);

    if(desktop_pref) { // desktop preference dialog
      iface.call("desktopPrefrences", QString(desktop_pref));
    }
    else if(find_files) { // file searching utility
      QStringList paths;
      if(file_names) {
        for(char** filename = file_names; *filename; ++filename) {
          QString path(*filename);
          paths.push_back(path);
        }
      }
      iface.call("findFiles", paths);
    }
    else if(show_pref) { // preferences dialog
      iface.call("preferences", QString(show_pref));
    }
    else if(wallpaper || wallpaper_mode) { // set wall paper
      iface.call("setWallpaper", QString(wallpaper), QString(wallpaper_mode));
    }
    else {
      if(!desktop && !desktop_off) {
        QStringList paths;
        if(file_names) {
          for(char** filename = file_names; *filename; ++filename) {
            QString path(*filename);
            paths.push_back(path);
          }
        }
        else
          paths.push_back(QDir::currentPath());
        // the function requires bool, but new_window is gboolean, the casting is needed.
        iface.call("launchFiles", paths, (bool)new_window);
      }
    }
  }

  // cleanup
  g_free(desktop_pref);
  g_free(show_pref);
  g_free(wallpaper);
  g_free(wallpaper_mode);
  g_free(profile);
  g_strfreev(file_names);

  return keepRunning;
}

int Application::exec() {

  if(!parseCommandLineArgs(QCoreApplication::argc(), QCoreApplication::argv()))
    return 0;

  if(daemonMode_) // keep running even when there is no window opened.
    setQuitOnLastWindowClosed(false);

  return QCoreApplication::exec();
}

void Application::onAboutToQuit() {
  qDebug("aboutToQuit");
}

void Application::commitData(QSessionManager& manager) {
  qDebug("commitData");
  settings_.save();
  QApplication::commitData(manager);
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
      int n = desktopWidget->numScreens();
      connect(desktopWidget, SIGNAL(workAreaResized(int)), SLOT(onWorkAreaResized(int)));
      connect(desktopWidget, SIGNAL(screenCountChanged(int)), SLOT(onScreenCountChanged(int)));
      desktopWindows_.reserve(n);
      for(int i = 0; i < n; ++i) {
        DesktopWindow* window = createDesktopWindow(i);
        desktopWindows_.push_back(window);
      }
    }
  }
  else {
    if(enableDesktopManager_) {
      disconnect(desktopWidget, SIGNAL(workAreaResized(int)), this, SLOT(onWorkAreaResized(int)));
      disconnect(desktopWidget, SIGNAL(screenCountChanged(int)), this, SLOT(onScreenCountChanged(int)));
      int n = desktopWindows_.size();
      for(int i = 0; i < n; ++i) {
        DesktopWindow* window = desktopWindows_.at(i);
        delete window;
      }
      desktopWindows_.clear();
    }
  }
  enableDesktopManager_ = enabled;
}

void Application::desktopPrefrences(QString page) {
  // TODO: show desktop preference window
  qDebug("show desktop preference window: %s", page.toUtf8().data());
}

void Application::findFiles(QStringList paths) {
  // TODO: add a file searching utility here.
  qDebug("findFiles");
}

void Application::launchFiles(QStringList paths, bool inNewWindow) {

  MainWindow* mainWin = new MainWindow();
  // open paths referred by paths
  // TODO: handle files
  QStringList::iterator it;
  for(it = paths.begin(); it != paths.end(); ++it) {
    QString& pathName = *it;
    FmPath* path = fm_path_new_for_path(pathName.toUtf8().constData());
    mainWin->addTab(path);
    fm_path_unref(path);
  }

  mainWin->resize(settings_.windowWidth(), settings_.windowHeight());
  mainWin->show();
}

void Application::preferences(QString page) {
  // open preference dialog
  if(!preferencesDialog_) {
    preferencesDialog_ = new PreferencesDialog(page);
  }
  else {
    // TODO: set page
  }
  preferencesDialog_.data()->show();
}

void Application::setWallpaper(QString path, QString modeString) {
  static const char* valid_wallpaper_modes[] = {"color", "stretch", "fit", "center", "tile"};
  settings_.setWallpaper(path);
  // settings_.setWallpaperMode();
  // TODO: update wallpaper
  qDebug("setWallpaper(\"%s\", \"%s\")", path.toUtf8().data(), modeString.toUtf8().data());
}

void Application::onWorkAreaResized(int num) {
  DesktopWindow* window = desktopWindows_.at(num);
  QRect rect = desktop()->screenGeometry(num);
  window->resize(rect.width(), rect.height());
  window->move(rect.x(), rect.y());
}

DesktopWindow* Application::createDesktopWindow(int screenNum) {
  DesktopWindow* window = new DesktopWindow();
  QRect rect = desktop()->screenGeometry(screenNum);
  window->setGeometry(rect);
  window->setWallpaperFile(settings_.wallpaper());
  window->setWallpaperMode(DesktopWindow::WallpaperStretch); // FIXME: set value from settings
  window->setForeground(settings_.desktopFgColor());
  window->updateWallpaper();
  window->show();
  return window;
}

void Application::onScreenCountChanged(int newCount) {
  int i;
  QDesktopWidget* desktopWidget = desktop();
  if(newCount > desktopWindows_.size()) {
    // add more desktop windows
    for(i = desktopWindows_.size(); i < newCount; ++i) {
      DesktopWindow* window = createDesktopWindow(i);
      desktopWindows_.push_back(window);
    }
  }
  else if(newCount < desktopWindows_.size()) {
    for(i = newCount; i < desktopWindows_.size(); ++i) {
      DesktopWindow* window = desktopWindows_.at(i);
      window->close();
    }
    desktopWindows_.resize(newCount);
  }
}

// called when Settings is changed to update UI
void Application::updateFromSettings() {

  Fm::IconTheme::setThemeName(settings_.iconThemeName());

  // update main windows and desktop windows
  QWidgetList windows = this->topLevelWidgets();
  QWidgetList::iterator it;
  for(it = windows.begin(); it != windows.end(); ++it) {
    QWidget* window = *it;
    if(window->inherits("PCManFM::MainWindow")) {
      MainWindow* mainWindow = static_cast<MainWindow*>(window);
      mainWindow->updateFromSettings(settings_);
    }
    else if(window->inherits("PCManFM::DesktopWindow")) {
      DesktopWindow* desktopWindow = static_cast<DesktopWindow*>(window);
      desktopWindow->updateFromSettings(settings_);
    }
  }

}


#include "application.moc"
