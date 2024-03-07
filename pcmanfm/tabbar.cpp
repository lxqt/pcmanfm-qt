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

const char* TabBar::tabDropped = "_pcmanfm_tab_dropped";

TabBar::TabBar(QWidget *parent):
    QTabBar(parent),
    dragStarted_(false),
    detachable_(true)
{
}

void TabBar::mousePressEvent(QMouseEvent *event) {
    QTabBar::mousePressEvent(event);
    if(detachable_) {
        if(event->button() == Qt::LeftButton && tabAt(event->pos()) > -1) {
            dragStartPosition_ = event->pos();
        }
        else {
            dragStartPosition_ = QPoint();
        }
        dragStarted_ = false;
    }
}

void TabBar::mouseMoveEvent(QMouseEvent *event)
{
    if(!detachable_) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    if(!dragStarted_ && !dragStartPosition_.isNull()
       && (event->pos() - dragStartPosition_).manhattanLength() >= QApplication::startDragDistance()) {
        dragStarted_ = true;
    }

    if((event->buttons() & Qt::LeftButton)
       && dragStarted_
       && !window()->geometry().contains(event->globalPosition().toPoint())) {
        if(currentIndex() == -1) {
            return;
        }

        // NOTE: To be on the safe side (especially under Wayland), we detach or drop the tab
        // only after finishing the DND; see MainWindow::dropEvent and the queued connection
        // to TabBar::tabDetached in MainWindow.

        QPointer<QDrag> drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setData(QStringLiteral("application/pcmanfm-qt-tab"), QByteArray());
        drag->setMimeData(mimeData);
        int N = count();
        Qt::DropAction dragged = drag->exec(Qt::MoveAction);
        if(dragged != Qt::MoveAction) {
            // The drop was not accepted (by any PCManFM-Qt window).
            // The tab will be detached if there is more than one tab.
            if(N > 1) {
                Q_EMIT tabDetached();
            }
            else {
                finishMouseMoveEvent();
            }
        }
        else {
            // Since another app can also accept this drop, we check if the object property
            // "_pcmanfm_tab_dropped" is set (by MainWindow::dropEvent) and detach the tab
            // if it is not; otherwise, the tab will be dropped into one of our windows.
            if(property(tabDropped).toBool()) {
                setProperty(tabDropped, QVariant()); // reset the property
            }
            else {
                if(N > 1)
                    Q_EMIT tabDetached();
                else
                    finishMouseMoveEvent();
            }
        }
        event->accept();
        drag->deleteLater();
    }
    else {
        QTabBar::mouseMoveEvent(event);
    }
}

void TabBar::finishMouseMoveEvent() {
    QMouseEvent finishingEvent(QEvent::MouseMove, QPoint(), QCursor::pos(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    mouseMoveEvent(&finishingEvent);
}

void TabBar::releaseMouse() {
    QMouseEvent releasingEvent(QEvent::MouseButtonRelease, QPoint(), QCursor::pos(), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    mouseReleaseEvent(&releasingEvent);
}

void TabBar::mouseReleaseEvent(QMouseEvent *event) {
    if(detachable_) { // reset drag info
        dragStarted_ = false;
        dragStartPosition_ = QPoint();
    }

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
    if(detachable_ && event->mimeData()->hasFormat(QStringLiteral("application/pcmanfm-qt-tab"))) {
        event->ignore();
    }
}

void TabBar::tabInserted(int index) {
    // WARNING: Qt6 has a bug that does not show the tabbar in our window on inserting the first
    // tab unless the tab layout is updated after its insertion. Usually, the tab text is reset
    // in MainWindow and the layout is updated, but it is also possible that the text is never
    // touched after the insertion. Therefore, we need the following workaround.
    if(!autoHide() && index == 0 && count() == 1) {
        updateGeometry();
    }
}

}
