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

namespace PCManFM {

class Application : public QApplication {
Q_OBJECT

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
  
protected Q_SLOTS:
  void onAboutToQuit();

  void onLastWindowClosed();
  void onSaveStateRequest(QSessionManager & manager);
  
protected:
  virtual void commitData(QSessionManager & manager);

private:
  Fm::Application fmApp_;
  Settings settings_;
};

}

#endif // PCMANFM_APPLICATION_H
