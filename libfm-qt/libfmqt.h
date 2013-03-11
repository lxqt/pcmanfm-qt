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


#ifndef FM_APPLICATION_H
#define FM_APPLICATION_H
#include <QtGlobal>
#include <QTranslator>
#include "icontheme.h"

namespace Fm {

class LIBFM_QT_API LibFmQt {
public:
  LibFmQt();
  ~LibFmQt();

  QTranslator* translator() {
    return &translator_;
  }
  static LibFmQt* instance();

protected:
  
private:
  IconTheme* iconTheme;
  QTranslator translator_;
};

}

#endif // FM_APPLICATION_H
