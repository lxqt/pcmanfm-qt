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

#ifndef PCMANFM_METADATA_H
#define PCMANFM_METADATA_H

#include <QObject>

class MetaData : public QObject {
  Q_OBJECT

public:
  explicit MetaData(const QString& path);
  virtual ~MetaData();

  int getWindowOriginX(bool& ok) const;
  int getWindowOriginY(bool& ok) const;
  int getWindowHeight(bool& ok) const;
  int getWindowWidth(bool& ok) const;

  void setWindowOriginX(int x);
  void setWindowOriginY(int y);
  void setWindowHeight(int height);
  void setWindowWidth(int width);

private:
  int getMetadataInt(const QString& path, const QString& attribute, bool& ok) const;
  void setMetadataInt(const QString& path, const QString& attribute, int value);

private:
  QString path_;
};

#endif // PCMANFM_METADATA_H
