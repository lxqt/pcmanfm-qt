/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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


#include "browsehistory.h"

using namespace Fm;

BrowseHistory::BrowseHistory():
  currentIndex_(0),
  maxCount_(10) {
}

BrowseHistory::~BrowseHistory() {
}

void BrowseHistory::add(FmPath* path, int scrollPos) {
  int lastIndex = size() - 1;
  if(currentIndex_ < lastIndex) {
    // if we're not at the last item, remove items after the current one.
    erase(begin() + currentIndex_ + 1, end());
  }

  if(size() + 1 > maxCount_) {
    // if there are too many items, remove the oldest one.
    // FIXME: what if currentIndex_ == 0? remove the last item instead?
    if(currentIndex_ == 0)
      remove(lastIndex);
    else {
      remove(0);
      --currentIndex_;
    }
  }
  // add a path and current scroll position to browse history
  append(BrowseHistoryItem(path, scrollPos));
  currentIndex_ = size() - 1;
}

void BrowseHistory::setCurrentIndex(int index) {
  if(index >= 0 && index < size()) {
    currentIndex_ = index;
    // FIXME: should we emit a signal for the change?
  }
}

bool BrowseHistory::canBackward() {
  return (currentIndex_ > 0);
}

int BrowseHistory::backward() {
  if(canBackward())
    --currentIndex_;
  return currentIndex_;
}

bool BrowseHistory::canForward() {
  return (currentIndex_ + 1 < size());
}

int BrowseHistory::forward() {
  if(canForward())
    ++currentIndex_;
  return currentIndex_;
}

void BrowseHistory::setMaxCount(int maxCount) {
  maxCount_ = maxCount;
  if(size() > maxCount) {
    // TODO: remove some items
  }
}
