/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef FM_ICONTHEME_H
#define FM_ICONTHEME_H

#include "libfmqtglobals.h"
#include <QIcon>
#include <QString>
#include "libfm/fm.h"

namespace Fm {
  
// NOTE:
// Qt seems to has its own QIcon pixmap caching mechanism internally.
// Besides, it also caches QIcon objects created by QIcon::fromTheme().
// So maybe we should not duplicate the work.
// See http://qt.gitorious.org/qt/qt/blobs/4.8/src/gui/image/qicon.cpp
// QPixmap QPixmapIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state).
// In addition, QPixmap is actually stored in X11 server, not client side.
// Hence maybe we should not cache too many pixmaps, I guess?
// Let's have Qt do its work and only translate GIcon to QIcon here.

// Nice article about QPixmap from KDE: http://techbase.kde.org/Development/Tutorials/Graphics/Performance

class LIBFM_QT_API IconTheme: public QObject {
  Q_OBJECT
public:
  IconTheme();
  ~IconTheme();

  static IconTheme* instance();
  static QIcon icon(FmIcon* fmicon);
  static QIcon icon(GIcon* gicon);
  
  void checkChanged(); // check if current icon theme name is changed
Q_SIGNALS:
  void changed(); // emitted when the name of current icon theme is changed

protected:
  bool eventFilter(QObject *obj, QEvent *event);
  static QIcon convertFromGIcon(GIcon* gicon);

protected:
  QIcon fallbackIcon_;
  QString currentThemeName_;
};

}

#endif // FM_ICONTHEME_H
