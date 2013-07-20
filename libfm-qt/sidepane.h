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


#ifndef FM_SIDEPANE_H
#define FM_SIDEPANE_H

#include "libfmqtglobals.h"
#include <QWidget>
#include <QVBoxLayout>
#include <qvarlengtharray.h>
#include "placesview.h"

namespace Fm {

class LIBFM_QT_API SidePane : public QWidget {
Q_OBJECT

public:
  explicit SidePane(QWidget* parent = 0);
  virtual ~SidePane();

  QSize iconSize() {
    return iconSize_;
  }

  void setIconSize(QSize size) {
    iconSize_ = size;
    if(placesView_)
      placesView_->setIconSize(size);
  }
  
  FmPath* currentPath();
  void setCurrentPath(FmPath* path);

Q_SIGNALS:
  void chdirRequested(int type, FmPath* path);

protected Q_SLOTS:
  void onPlacesViewChdirRequested(int type, FmPath* path);
  
private:
  PlacesView* placesView_;
  QVBoxLayout* verticalLayout;
  QSize iconSize_;
};

}

#endif // FM_SIDEPANE_H
