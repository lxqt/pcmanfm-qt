/*
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

#ifndef _LIBFM_QT_GLOBALS_
#define _LIBFM_QT_GLOBALS_

#include <QtGlobal>

#ifdef LIBFM_QT_COMPILATION
    #define LIBFM_QT_API    Q_DECL_EXPORT
#else
    #define LIBFM_QT_API    Q_DECL_IMPORT
#endif

#endif
