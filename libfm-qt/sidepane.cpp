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


#include "sidepane.h"

using namespace Fm;

SidePane::SidePane(QWidget* parent):
  QWidget(parent),
  iconSize_(24, 24) {

  verticalLayout = new QVBoxLayout(this);
  verticalLayout->setContentsMargins(0, 0, 0, 0);
  placesView_ = new Fm::PlacesView(this);
  connect(placesView_, SIGNAL(chdirRequested(int,FmPath*)), SLOT(onPlacesViewChdirRequested(int,FmPath*)));
  verticalLayout->addWidget(placesView_);
}

SidePane::~SidePane() {
  qDebug("delete SidePane");
}

void SidePane::onPlacesViewChdirRequested(int type, FmPath* path) {
  Q_EMIT chdirRequested(type, path);
}

// FIXME: in the future, we can have different modes for side pane, not just places view.
FmPath* SidePane::currentPath() {
  if(placesView_)
    return placesView_->currentPath();
  return NULL;
}

void SidePane::setCurrentPath(FmPath* path) {
  if(placesView_) {
    placesView_->setCurrentPath(path);
  }
}


#include "sidepane.moc"
