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


#include "desktopsettings.h"
#include <QIcon>
#include <QSettings>
#include <QTimer>
#include <QX11Info> // FIXME: deprecated in Qt5
#include <QApplication>
#include <QWidget>
#include <stdlib.h>
#include <glib.h>

// X11 related header files cannot be put before Qt ones.
// They contains some macros which causes name clashes with
// some Qt enums and causes compiler errors. :-(
#include "xsettings/xsettings-client.h"

using namespace Xdg;

DesktopSettings::DesktopSettings():
  configFileWatcher_(NULL),
  delayEmitTimeout_(NULL),
  xsettings_(NULL) {

  watchedWindows_.reserve(2);

  detectDesktopEnvironment();
  loadSettings();
}

DesktopSettings::~DesktopSettings() {
  if(configFileWatcher_)
    delete configFileWatcher_;
  if(xsettings_)
    xsettings_client_destroy(xsettings_);
  if(delayEmitTimeout_)
    delete delayEmitTimeout_;
}

void DesktopSettings::detectDesktopEnvironment() {
  const char* deName = getenv("XDG_CURRENT_DESKTOP");
  if(deName && *deName)
    // LXDE and Razor-Qt both export this environment variable
    // XFCE in some distros also does this
    // The values they set are: LXDE, Razor, and XFCE
    desktopEnvironment_ = deName;
  else {
    if(getenv("KDE_FULL_SESSION"))
      desktopEnvironment_ = "KDE";
    else if(getenv("GNOME_DESKTOP_SESSION_ID"))
      desktopEnvironment_ = "GNOME";
    else if(getenv("MATE_DESKTOP_SESSION_ID"))
      desktopEnvironment_ = "Mate";
    else if(const char* session = getenv("DESKTOP_SESSION")) {
      if(strcmp(session, "gnome") == 0)
        session = "GNOME";
      else if(strncmp(session, "xfce", 4) == 0 || strncmp(session, "Xfce", 4) == 0)
        session = "XFCE";
    }
  }
}

void DesktopSettings::loadSettings() {
  if(desktopEnvironment_ == "LXDE"
    || desktopEnvironment_ == "GNOME"
    || desktopEnvironment_ == "Mate"
    || desktopEnvironment_ == "XFCE") {
    // read from Xsettings
    xsettings_ = xsettings_client_new(QX11Info::display(), 0,
                                      XSettingsNotifyFunc(xsettingsNotify),
                                      XSettingsWatchFunc(xsettingsWatch), this);
    // read settings
    if(xsettings_) {
      XSettingsSetting* setting;
      if(xsettings_client_get_setting(xsettings_, "Net/IconThemeName", &setting) == XSETTINGS_SUCCESS) {
        iconThemeName_ = setting->data.v_string;
        xsettings_setting_free(setting);
      }
    }
  }
  else if(desktopEnvironment_ == "Razor") {
    // read razor config file
    configFilePath_ = g_get_user_config_dir();
    configFilePath_ += "/razor/razor.conf";
    // monitor for changes of the config file
    configFileWatcher_ = new QFileSystemWatcher();
    configFileWatcher_->addPath(configFilePath_);
    connect(configFileWatcher_, SIGNAL(fileChanged(QString)), SLOT(onConfigFileChanged(QString)));

    {
      QSettings settings(configFilePath_, QSettings::IniFormat);
      iconThemeName_ = settings.value("icon_theme").toString();
    }

    if(iconThemeName_.isEmpty()) {
      const char* const* dirs = g_get_system_config_dirs();
      for(const char* const* dir = dirs; *dir; ++dir) {
        QString path = *dir;
        path += "/razor/razor.conf";
        QSettings settings(path, QSettings::IniFormat);
        iconThemeName_ = settings.value("icon_theme").toString();
        if(!iconThemeName_.isEmpty())
          break;
      }
    }
    if(iconThemeName_.isEmpty())
      iconThemeName_ = "oxygen";
  }
  else if(desktopEnvironment_ == "KDE") {
    // TODO: read KDE config file? How to do it?
  }
  QIcon::setThemeName(iconThemeName_);
}

void DesktopSettings::emitChanged() {
  qDebug("emit");
  Q_EMIT changed();
  delete delayEmitTimeout_;
  delayEmitTimeout_ = NULL;

  // repaint existing windows to reflect the changes
  Q_FOREACH(QWidget* widget, QApplication::topLevelWidgets()) {
    widget->update();
  }
}

// only emit one changed() signal for serial changes
void DesktopSettings::queueEmitChanged() {
  if(delayEmitTimeout_)
    delayEmitTimeout_->start(400);
  else {
    delayEmitTimeout_ = new QTimer();
    delayEmitTimeout_->setSingleShot(true);
    delayEmitTimeout_->start(400);
    connect(delayEmitTimeout_, SIGNAL(timeout()), SLOT(emitChanged()));
  }
}

void DesktopSettings::xsettingsNotify(const char* name, int action, XSettingsSetting* setting, DesktopSettings* pThis) {
  if(strcmp(name, "Net/IconThemeName") == 0) {
    if(action == XSETTINGS_ACTION_DELETED)
      pThis->iconThemeName_.clear();
    else {
      pThis->iconThemeName_ = setting->data.v_string;
      QIcon::setThemeName(pThis->iconThemeName_);
    }
  }
  // delay signal emission, only emit one changed() signal for serial changes
  pThis->queueEmitChanged();
}

void DesktopSettings::xsettingsWatch(unsigned long window, int is_start, long int mask, DesktopSettings* pThis) {
  if(is_start)
    pThis->watchedWindows_.append(window);
  else {
    int pos = pThis->watchedWindows_.indexOf(window);
    if(pos != -1)
      pThis->watchedWindows_.remove(pos);
  }
}

// This will be removed in Qt 5 and I haven't find a replacement yet.
// However, when everybody is using Qt 5, it's also time to deprecate Xsettings.
// So, it's not that bad. We may need to support dconf then.
bool DesktopSettings::x11EventFilter(XEvent* event) {
  if(xsettings_ && watchedWindows_.contains(event->xany.window)) {
    return bool(xsettings_client_process_event(xsettings_, event));
  }
  return false;
}

void DesktopSettings::onConfigFileChanged(QString path) {
  QSettings settings(configFilePath_, QSettings::IniFormat);
  iconThemeName_ = settings.value("icon_theme").toString();
  QIcon::setThemeName(iconThemeName_);
  queueEmitChanged();
}
