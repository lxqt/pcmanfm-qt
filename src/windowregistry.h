/*
 *    Copyright (C) 2021 Chris Moore <chris@mooreonline.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef WINDOWREGISTRY_H
#define WINDOWREGISTRY_H

#include <QObject>
#include <QSet>
#include <QString>

class WindowRegistry : public QObject {
  Q_OBJECT

public:
  virtual ~WindowRegistry();

  // singleton function
  static WindowRegistry& instance()
  {
    static WindowRegistry instance;
    return instance;
  }

  void registerPath(const QString& path);
  void updatePath(const QString& fromPath, const QString& toPath);
  void deregisterPath(const QString& path);

  /*
   * if the window is in the registry, raise the window
   * and return true; otherwise return false
   */
  bool checkPathAndRaise(const QString& path);

Q_SIGNALS:
  void raiseWindow(const QString& path);

private: // functions
  explicit WindowRegistry(QObject* parent = nullptr);

private: // data
  QSet<QString> registry_; // set of paths of open windows
};

#endif // WINDOWREGISTRY_H
