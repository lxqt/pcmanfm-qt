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

using namespace PCManFM;

Application::Application(int& argc, char** argv):
  QApplication(argc, argv),
  fmApp_(),
  settings_() {

  connect(this, SIGNAL(aboutToQuit()), SLOT(onAboutToQuit()));
  settings_.load();

  Fm::IconTheme::setThemeName(settings_.iconThemeName());

  // TODO: how to parse command line arguments in Qt?
  // options are:
  // 1. QCommandLine: http://xf.iksaif.net/dev/qcommandline.html
  // 2. http://code.google.com/p/qtargparser/
  
  // TODO: implement single instance
  // 1. use dbus?
  // 2. Try QtSingleApplication http://doc.qt.digia.com/solutions/4/qtsingleapplication/qtsingleapplication.html
}

Application::~Application() {

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

#include "application.moc"
