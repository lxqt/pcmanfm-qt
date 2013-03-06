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


#ifndef PCMANFM_APPLICATION_H
#define PCMANFM_APPLICATION_H

#include <QApplication>
#include "settings.h"
#include "../libfm-qt/application.h"
#include <QVector>
#include <QWeakPointer>

namespace PCManFM {

class DesktopWindow;
class PreferencesDialog;
class DesktopPreferencesDialog;

class Application : public QApplication {
Q_OBJECT
Q_PROPERTY(bool desktopManagerEnabled READ desktopManagerEnabled)

public:
  Application(int& argc, char** argv);
  virtual ~Application();

  int exec();

  Settings& settings() {
    return settings_;
  }
  
  Fm::Application& fmApp() {
    return fmApp_;
  }

  // public interface exported via dbus
  void launchFiles(QStringList paths, bool inNewWindow);
  void setWallpaper(QString path, QString modeString);
  void preferences(QString page);
  void desktopPrefrences(QString page);
  void desktopManager(bool enabled);
  void findFiles(QStringList paths);

  bool desktopManagerEnabled() {
    return enableDesktopManager_;
  }

  void updateFromSettings();
  void updateDesktopsFromSettings();
  
protected Q_SLOTS:
  void onAboutToQuit();

  void onLastWindowClosed();
  void onSaveStateRequest(QSessionManager & manager);
  void onScreenResized(int num);
  void onScreenCountChanged(int newCount);

protected:
  virtual void commitData(QSessionManager & manager);
  bool parseCommandLineArgs(int argc, char** argv);
  DesktopWindow* createDesktopWindow(int screenNum);

private:
  bool isPrimaryInstance;
  Fm::Application fmApp_;
  Settings settings_;
  QString profileName;
  bool daemonMode_;
  bool enableDesktopManager_;
  QVector<DesktopWindow*> desktopWindows_;
  QWeakPointer<PreferencesDialog> preferencesDialog_;
  QWeakPointer<DesktopPreferencesDialog> desktopPreferencesDialog_;
};

}

#endif // PCMANFM_APPLICATION_H
