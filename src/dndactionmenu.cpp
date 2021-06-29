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


#include "dndactionmenu.h"

using namespace Fm;

DndActionMenu::DndActionMenu(QWidget* parent): QMenu(parent) {
  copyAction = addAction(QIcon::fromTheme("edit-copy"), tr("Copy here"));
  moveAction = addAction(tr("Move here"));
  linkAction = addAction(tr("Create symlink here"));
  addSeparator();
  cancelAction = addAction(tr("Cancel"));
}

DndActionMenu::~DndActionMenu() {

}

Qt::DropAction DndActionMenu::askUser(QPoint pos) {
  Qt::DropAction result;
  DndActionMenu menu;
  QAction* action = menu.exec(pos);
  if(action == menu.copyAction)
    result = Qt::CopyAction;
  else if(action == menu.moveAction)
    result = Qt::MoveAction;
  else if(action == menu.linkAction)
    result = Qt::LinkAction;
  else
    result = Qt::IgnoreAction;
  return result;
}
