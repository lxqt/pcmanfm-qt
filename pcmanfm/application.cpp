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

#include <QCommandLine>

#include "applicationadaptor.h"

namespace PCManFM {

  class CommandLineData {
  public:
    CommandLineData():
      daemonMode(false),
      desktop(false),
      desktopOff(false),
      newWindow(false),
      findFiles(false) {      
    }

    QString profileName;
    bool daemonMode;
    bool desktop;
    bool desktopOff;
    QString desktopPref;
    QString wallpaper;
    QString wallpaperMode;
    QString showPref;
    bool newWindow;
    bool findFiles;
    QStringList paths;
  };
  
};

using namespace PCManFM;
static const char* serviceName = "org.pcmanfm.PCManFM";
static const char* ifaceName = "org.pcmanfm.Application";

Application::Application(int& argc, char** argv):
  QApplication(argc, argv),
  fmApp_(),
  settings_(),
  profileName("default"),
  daemonMode_(false),
  enableDesktopManager_(false),
  cmdLineData_(NULL) {

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


bool Application::parseCommandLineArgs(int argc, char** argv) {
  // It's really a shame that the great Qt library does not come
  // with any command line parser. Let's use QCommandLine.
  // TODO: patch QCommandLine so it can accept empty shortnames.
  const QCommandLineConfigEntry conf[] = {
    {QCommandLine::Option, 'p', "profile", tr("Name of configuration profile"), QCommandLine::Optional},
    {QCommandLine::Switch, 'd', "daemon-mode", tr("Run PCManFM as a daemon"), QCommandLine::Optional},
    // options that are acceptable for every instance of pcmanfm and will be passed through IPC.
    {QCommandLine::Switch, 't', "desktop", tr("Launch desktop manager"), QCommandLine::Optional},
    {QCommandLine::Switch, 'o', "desktop-off", tr("Turn off desktop manager if it's running"), QCommandLine::Optional},
    {QCommandLine::Option, 'e', "desktop-pref", tr("Open desktop preference dialog"), QCommandLine::Optional},
    {QCommandLine::Option, 'w', "set-wallpaper", tr("Set desktop wallpaper from image FILE"), QCommandLine::Optional},
    // don't translate list of modes in description, please
    {QCommandLine::Option, 'm', "wallpaper-mode", tr("Set mode of desktop wallpaper. MODE=(color|stretch|fit|center|tile)"), QCommandLine::Optional},
    {QCommandLine::Option, 'r', "show-pref", tr("Open Preferences dialog on the page N"), QCommandLine::Optional},
    {QCommandLine::Switch, 'n', "new-window", tr("Open new window"), QCommandLine::Optional},
    {QCommandLine::Switch, 'f', "find-files", tr("Open Find Files utility"), QCommandLine::Optional},
    {QCommandLine::Param, '\0', "paths", ("File or folder paths to opern"), QCommandLine::OptionalMultiple},
    {QCommandLine::None, '\0', NULL, NULL, QCommandLine::Default}
  };
  QCommandLine cmdLine(this);
  cmdLine.setConfig(conf);
  cmdLine.enableVersion(true); // enable -v // --version
  cmdLine.enableHelp(true); // enable -h / --help
  // why Qt-using developers like to use signal/slot so much?
  // in this case, an array of callback functions works much more efficient.
  // Anyway, this is not a performance critical part.
  // If someday other parts are finished, maybe we can do our own command line parsing.
  connect(&cmdLine, SIGNAL(switchFound(QString)), SLOT(onCmdLineSwitch(QString)));
  connect(&cmdLine, SIGNAL(optionFound(QString,QVariant)), SLOT(onCmdLineOption(QString,QVariant)));
  connect(&cmdLine, SIGNAL(paramFound(QString,QVariant)), SLOT(onCmdLineParam(QString,QVariant)));
  connect(&cmdLine, SIGNAL(parseError(QString)), SLOT(onCmdLineError(QString)));

  CommandLineData data;
  cmdLineData_ = &data;
  cmdLine.parse();
  cmdLineData_ = NULL;

  if(isPrimaryInstance) {
    qDebug("isPrimaryInstance");

    if(data.daemonMode)
      daemonMode_ = true;
    if(!data.profileName.isEmpty())
      profileName = data.profileName;

    // load settings
    settings_.load(profileName);

    qDebug("HERE");
    // desktop icon management
    if(data.desktop)
      desktopManager(true);
    else if(data.desktopOff)
      desktopManager(false);

    qDebug("HERE");
    if(!data.desktopPref.isEmpty()) // desktop preference dialog
      desktopPrefrences(data.desktopPref);
    else if(data.findFiles) // file searching utility
      findFiles(data.paths);
    else if(!data.showPref.isEmpty()) // preferences dialog
      preferences(data.showPref);
    else if(!data.wallpaper.isEmpty()) { // set wall paper
      setWallpaper(data.wallpaper, data.wallpaperMode);
    }
    else {
      if(data.paths.isEmpty())
        data.paths.push_back(QDir::currentPath());
      launchFiles(data.paths, data.newWindow);
      return true;
    }
  }
  else {
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusInterface* iface = new QDBusInterface(serviceName,
                                               "/Application",
                                               ifaceName,
                                               dbus,
                                               this);
    if(data.desktop)
      iface->call("desktopManager", true);
    else if(data.desktopOff)
      iface->call("desktopManager", false);

    if(!data.desktopPref.isEmpty()) // desktop preference dialog
      iface->call("desktopPrefrences", data.desktopPref);
    else if(data.findFiles) // file searching utility
      iface->call("findFiles", data.paths);
    else if(!data.showPref.isEmpty()) // preferences dialog
      iface->call("preferences", data.showPref);
    else if(!data.wallpaper.isEmpty()) // set wall paper
      iface->call("setWallpaper", data.wallpaper, data.wallpaperMode);
    else {
      if(data.paths.isEmpty())
          data.paths.push_back(QDir::currentPath());
      iface->call("launchFiles", data.paths, data.newWindow);
    }
  }
  return false;
}

int Application::exec() {

  if(!parseCommandLineArgs(QCoreApplication::argc(), QCoreApplication::argv()))
    return 0;

  MainWindow mainWin;
  // The desktop icons window
  // PCManFM::DesktopWindow desktopWindow;
  // desktopWindow.show();

  mainWin.resize(640, 480);
  mainWin.show();

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

void Application::onCmdLineError(const QString error) {
  // TODO: print error message
  exit(1);
}

void Application::onCmdLineOption(const QString& name, const QVariant& value) {
  if(name == "profile") {
    profileName = value.toString();
  }
  else if(name == "desktop-pref") {
    cmdLineData_->desktopPref = value.toString();
  }
  else if(name == "set-wallpaper") {
    cmdLineData_->wallpaper = value.toString();
  }
  else if(name == "wallpaper-mode") {
    cmdLineData_->wallpaperMode;
  }
  else if(name == "show-pref") {
    cmdLineData_->showPref = value.toString();
  }
}

void Application::onCmdLineSwitch(const QString& name) {
  if(name == "daemon-mode") {
    cmdLineData_->daemonMode = true;
  }
  else if(name == "desktop") {
    cmdLineData_->desktop = true;
  }
  else if(name == "desktop-off") {
    cmdLineData_->desktopOff = true;
  }
  else if(name == "new-window") {
    cmdLineData_->newWindow = true;
  }
  else if(name == "find-files") {
    cmdLineData_->findFiles = true;
  }
}

void Application::onCmdLineParam(const QString& name, const QVariant& value) {
  if(name == "paths") {
    cmdLineData_->paths = value.toStringList();
  }
}

void Application::desktopManager(bool enabled) {
  // TODO: turn on or turn off desktpo management (desktop icons & wallpaper)
}

void Application::desktopPrefrences(QString page) {
  // TODO: show desktop preference window
}

void Application::findFiles(QStringList paths) {
  // TODO: add a file searching utility here.
}

void Application::launchFiles(QStringList paths, bool inNewWindow) {
  // TODO: open paths referred by paths
  QStringList::iterator it;
  for(it = paths.begin(); it != paths.end(); ++it) {
    QString& pathName = *it;
    qDebug("launch: %s, %d", pathName.toUtf8().data(), (int)inNewWindow);
  }
}

void Application::preferences(QString page) {
  // TODO: open preference dialog
}

void Application::setWallpaper(QString path, QString modeString) {
  static const char* valid_wallpaper_modes[] = {"color", "stretch", "fit", "center", "tile"};
  settings_.setWallpaper(path);
  // settings_.setWallpaperMode();
  // TODO: update wallpaper
}

#include "application.moc"
