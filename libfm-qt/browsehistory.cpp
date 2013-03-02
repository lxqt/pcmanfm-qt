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


#include "browsehistory.h"

using namespace Fm;

BrowseHistory::BrowseHistory():
  currentIndex_(0),
  maxCount_(10) {
}

BrowseHistory::~BrowseHistory() {
}

void BrowseHistory::add(FmPath* path, int scrollPos) {
  if(currentIndex_ < size() - 1) {
    int n = size() - currentIndex_ - 1;
    remove(currentIndex_, n);
  }
  if(size() + 1 > maxCount_) {
    remove(0);
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
