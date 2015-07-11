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


#ifndef BOOKMARKACTION_H
#define BOOKMARKACTION_H

#include "libfmqtglobals.h"
#include <QAction>
#include <libfm/fm.h>

namespace Fm {

// action used to create bookmark menu items
class LIBFM_QT_API BookmarkAction : public QAction {
public:
  explicit BookmarkAction(FmBookmarkItem* item, QObject* parent = 0);

  virtual ~BookmarkAction() {
    if(item_)
      fm_bookmark_item_unref(item_);
  }

  FmBookmarkItem* bookmark() {
    return item_;
  }

  FmPath* path() {
    return item_->path;
  }

private:
  FmBookmarkItem* item_;
};

}

#endif // BOOKMARKACTION_H
