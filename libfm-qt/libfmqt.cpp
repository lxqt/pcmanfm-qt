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

#include <libfm/fm.h>
#include "libfmqt.h"
#include <QLocale>

using namespace Fm;

LibFmQt* theApp = NULL;

LibFmQt::LibFmQt() {
  // TODO: only single instance is allowed, show a warning
  Q_ASSERT(!theApp);

  theApp = this;
  g_type_init();
  fm_init(NULL);

  // turn on glib debug message
  // g_setenv("G_MESSAGES_DEBUG", "all", true);

  iconTheme = new IconTheme();
  thumbnailLoader = new ThumbnailLoader();
  translator_.load("libfm-qt_" + QLocale::system().name(), LIBFM_DATA_DIR "/translations");
}

LibFmQt::~LibFmQt() {
  delete iconTheme;
  delete thumbnailLoader;
  fm_finalize();
}

LibFmQt* LibFmQt::instance() {
  return theApp;
}

