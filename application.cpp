/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

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
#include "application.h"

using namespace Fm;

Application* theApp = NULL;

Application::Application() {
  if(!theApp) {
    theApp = this;
    g_type_init();
    fm_init(NULL);
    
    iconTheme = new IconTheme();
  }
  else {
    // TODO: only single instance is allowed, show a warning
  }
}

Application::~Application() {
  if(iconTheme)
    delete iconTheme;
  fm_finalize();
}

