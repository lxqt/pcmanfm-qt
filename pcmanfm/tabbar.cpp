/*

	Copyright (C) 2014  Kuzma Shapran <kuzma.shapran@gmail.com>

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


#include "tabbar.h"
#include <QPointer>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>

namespace PCManFM {

TabBar::TabBar(QWidget *parent):
  QTabBar(parent)
{
}

void TabBar::mousePressEvent(QMouseEvent *event) {
  QTabBar::mousePressEvent (event);
  if(event->button() == Qt::LeftButton
     && tabAt(event->pos()) > -1) {
    dragStartPosition_ = event->pos();
  }
  dragStarted_ = false;
}

void TabBar::mouseMoveEvent(QMouseEvent *event)
{
  if(!dragStartPosition_.isNull()
     && (event->pos() - dragStartPosition_).manhattanLength() < QApplication::startDragDistance()) {
    dragStarted_ = true;
  }

  if((event->buttons() & Qt::LeftButton)
     && dragStarted_
     && !window()->geometry().contains(event->globalPos())) {
    if(currentIndex() == -1)
      return;

    QPointer<QDrag> drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/pcmanfm-qt-tab", QByteArray());
    drag->setMimeData(mimeData);
    Qt::DropAction dragged = drag->exec();
    if(dragged == Qt::IgnoreAction) { // a tab is dropped outside all windows
      if(count() > 1)
        Q_EMIT tabDetached();
      else
        finishMouseMoveEvent();
      event->accept();
    }
    else if(dragged == Qt::MoveAction) // a tab is dropped into another window
      event->accept();
    drag->deleteLater();
  }
  else
    QTabBar::mouseMoveEvent(event);
}

void TabBar::finishMouseMoveEvent() {
  QMouseEvent finishingEvent(QEvent::MouseMove, QPoint(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
  mouseMoveEvent(&finishingEvent);
}

void TabBar::releaseMouse() {
  QMouseEvent releasingEvent(QEvent::MouseButtonRelease, QPoint(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
  mouseReleaseEvent(&releasingEvent);
}

void TabBar::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::MiddleButton) {
    int index = tabAt(event->pos());
    if (index != -1) {
      Q_EMIT tabCloseRequested(index);
    }
  }
  QTabBar::mouseReleaseEvent(event);
}

// Let the main window receive dragged tabs!
void TabBar::dragEnterEvent(QDragEnterEvent *event) {
  if(event->mimeData()->hasFormat("application/pcmanfm-qt-tab"))
    event->ignore();
}

}
