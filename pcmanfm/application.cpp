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
#include <QCommandLine>

using namespace PCManFM;

Application::Application(int& argc, char** argv):
  QApplication(argc, argv),
  fmApp_(),
  settings_(),
  profileName("default") {
    
  parseCommandLine(argc, argv);

  connect(this, SIGNAL(aboutToQuit()), SLOT(onAboutToQuit()));
  settings_.load(profileName);

  Fm::IconTheme::setThemeName(settings_.iconThemeName());
}

Application::~Application() {

}

void Application::parseCommandLine(int argc, char** argv) {
  
  // QStringList* args = arguments();

  // TODO: implement single instance
  // 1. use dbus?
  // 2. Try QtSingleApplication http://doc.qt.digia.com/solutions/4/qtsingleapplication/qtsingleapplication.html

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
    {QCommandLine::Switch, 'n', "new-win", tr("Open new window"), QCommandLine::Optional},
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
  cmdLine.parse();
}

int Application::exec() {
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

}

void Application::onCmdLineOption(const QString& name, const QVariant& value) {
  if(name == "profile") {
    profileName = value.toString();
  }
  else if(name == "desktop-pref") {
    
  }
  else if(name == "set-wallpaper") {
    
  }
  else if(name == "wallpaper-mode") {
    
  }
  else if(name == "show-pref") {
    
  }
}

void Application::onCmdLineSwitch(const QString& name) {
  if(name == "daemon-mode") {
    
  }
  else if(name == "desktop") {
    
  }
  else if(name == "desktop-off") {
  
  }
  else if(name == "new-win") {
    
  }
  else if(name == "find-files") {
    
  }  
}

void Application::onCmdLineParam(const QString& name, const QVariant& value) {
  if(name == "paths") {
    
  }
}


#include "application.moc"
