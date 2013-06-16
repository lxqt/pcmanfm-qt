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


#ifndef XDG_DESKTOPSETTINGS_H
#define XDG_DESKTOPSETTINGS_H

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>
#include <QApplication> // for x11EventFilter
#include <QVector>
#include <QTimer>

// We cannot include #include <X11/Xlib.h> along with Qt headers due to some name clashes
// caused by various macros defined in Xlib.
typedef struct _XSettingsClient XSettingsClient;
typedef struct _XSettingsSetting XSettingsSetting;

namespace Xdg {

// cross-desktop global settings, such as icon theme name, ...
class DesktopSettings : public QObject {
  Q_OBJECT
public:

  DesktopSettings();
  virtual ~DesktopSettings();
  
  QString iconThemeName() const {
    return iconThemeName_;
  }

  QString desktopEnvironment() const {
    return desktopEnvironment_;
  }

  // FIXME: this will stop working in Qt 5 :-(
  bool x11EventFilter(XEvent* event); // needs to be called by QApplication

Q_SIGNALS:
  void changed();

private:
  void detectDesktopEnvironment();
  void loadSettings();

  static void xsettingsNotify(const char *name,
                              int action, // we replace enum with int here
                              XSettingsSetting *setting, DesktopSettings* pThis);
  static void xsettingsWatch(unsigned long window, int is_start, long mask, DesktopSettings* pThis);

  void queueEmitChanged();

private Q_SLOTS:
  void onConfigFileChanged(QString path);
  void emitChanged();
  
private:
  QString desktopEnvironment_;
  QString iconThemeName_;
  QString configFilePath_;
  QFileSystemWatcher* configFileWatcher_;
  XSettingsClient* xsettings_;
  QVector<unsigned long> watchedWindows_;
  QTimer* delayEmitTimeout_;
};

};

#endif // XDG_DESKTOPSETTINGS_H
