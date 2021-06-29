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

#include <QMap>
#include <QObject>

class MetaData : public QObject {
  Q_OBJECT

public:

  enum FolderView {
    Icons = 1,
    Compact = 2,
    List = 3,
    Thumbnail = 4,
    Columns = 5
  };

  enum SortItem {
    FileName = 1,
    FileType = 2,
    FileSize = 3,
    ModifiedTime = 4,
    Owner = 5
  };

  enum SortOrder {
    Ascending = 1,
    Descending = 2
  };

  enum SortCase {
    CaseSensitive = 1,
    NotCaseSensitive = 2
  };

  enum SortFolderFirst {
    FoldersFirst = 1,
    NotFoldersFirst = 2
  };

  enum Filter {
    FilterActive = 1,
    FilterInactive = 2
  };

  explicit MetaData(const QString& path);
  virtual ~MetaData();

  int getWindowOriginX(bool& ok) const;
  int getWindowOriginY(bool& ok) const;
  int getWindowHeight(bool& ok) const;
  int getWindowWidth(bool& ok) const;

  FolderView getWindowView(bool& ok) const;
  SortItem getWindowSortItem(bool& ok) const;
  SortOrder getWindowSortOrder(bool& ok) const;
  SortCase getWindowSortCase(bool& ok) const;
  SortFolderFirst getWindowSortFolderFirst(bool& ok) const;
  Filter getWindowFilter(bool& ok) const;

  void setWindowOriginX(int x);
  void setWindowOriginY(int y);
  void setWindowHeight(int height);
  void setWindowWidth(int width);

  void setWindowView(FolderView view);
  void setWindowSortItem(SortItem sortItem);
  void setWindowSortOrder(SortOrder sortOrder);
  void setWindowSortCase(SortCase sortCase);
  void setWindowSortFolderFirst(SortFolderFirst sortFolderFirst);
  void setWindowFilter(Filter filter);

private:
  int getMetadataInt(const QString& path, const QString& attribute, bool& ok) const;
  void setMetadataInt(const QString& path, const QString& attribute, int value);
  void loadDirInfo();
  void saveDirInfo();
  bool dirInfoDisabledForPath() const;

private:
  QString path_;
  QMap<QString, QVariant> windowAttributes_;
};

#endif // PCMANFM_METADATA_H
