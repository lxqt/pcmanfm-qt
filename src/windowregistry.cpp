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

#include "windowregistry.h"

WindowRegistry::~WindowRegistry() {
  // do nothing
}

void WindowRegistry::registerPath(const QString& path)
{
  registry_.insert(path);
}

void WindowRegistry::updatePath(const QString& fromPath, const QString& toPath)
{
  registry_.remove(fromPath);
  registry_.insert(toPath);
}

void WindowRegistry::deregisterPath(const QString& path)
{
  registry_.remove(path);
}

bool WindowRegistry::checkPathAndRaise(const QString& path)
{
  if (registry_.contains(path)) {
    Q_EMIT raiseWindow(path);
    return true;
  }
  return false;
}

WindowRegistry::WindowRegistry(QObject* parent):
  QObject(),
  registry_() {
  // do nothing
}
