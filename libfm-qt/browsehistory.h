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


#ifndef FM_BROWSEHISTORY_H
#define FM_BROWSEHISTORY_H

#include <QVector>
#include <libfm/fm.h>

namespace Fm {

// class used to story browsing history of folder views
// We use this class to replace FmNavHistory provided by libfm since
// the original Libfm API is hard to use and confusing.

class LIBFM_QT_API BrowseHistoryItem {
public:

  BrowseHistoryItem():
    path_(NULL),
    scrollPos_(0) {
  }

  BrowseHistoryItem(FmPath* path, int scrollPos = 0):
    path_(fm_path_ref(path)),
    scrollPos_(scrollPos) {
  }

  BrowseHistoryItem(const BrowseHistoryItem& other):
    path_(other.path_ ? fm_path_ref(other.path_) : NULL),
    scrollPos_(other.scrollPos_) {
  }
  
  ~BrowseHistoryItem() {
    if(path_)
      fm_path_unref(path_);
  }
  
  FmPath* path() {
    return path_;
  }

  int scrollPos() {
    return scrollPos_;
  }

  void setScrollPos(int pos) {
    scrollPos_ = pos;
  }

private:
  FmPath* path_;
  int scrollPos_;
  // TODO: we may need to store current selection as well. reserve room for furutre expansion.
  void* reserved1;
  void* reserved2;
};

class LIBFM_QT_API BrowseHistory : public QVector<BrowseHistoryItem> {

public:
  BrowseHistory();
  virtual ~BrowseHistory();

  int currentIndex() {
    return currentIndex_;
  }
  void setCurrentIndex(int index);

  FmPath* currentPath() {
    return at(currentIndex_).path();
  }

  int currentScrollPos() {
    return at(currentIndex_).scrollPos();
  }

  BrowseHistoryItem& currentItem() {
    return operator[](currentIndex_);
  }

  void add(FmPath* path, int scrollPos = 0);

  bool canForward();

  bool canBackward();

  int backward();

  int forward();

  int maxCount() {
    return maxCount_;
  }

  void setMaxCount(int maxCount);

private:
  int currentIndex_;
  int maxCount_;
};

}

#endif // FM_BROWSEHISTORY_H
