/*
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

#include "statusbar.h"

#define MESSAGE_DELAY 250

namespace PCManFM {

StatusBar::StatusBar(QWidget *parent):
  QStatusBar(parent),
  lastTimeOut_(0) {
    statusLabel_ = new QLabel();
    statusLabel_->setIndent(4);
    addWidget(statusLabel_);

    messageTimer_ = new QTimer (this);
    messageTimer_->setSingleShot(true);
    messageTimer_->setInterval(MESSAGE_DELAY);
    connect(messageTimer_, &QTimer::timeout, this, &StatusBar::reallyShowMessage);
}

StatusBar::~StatusBar() {
    if(messageTimer_) {
        messageTimer_->stop();
        delete messageTimer_;
    }
}

void StatusBar::showMessage(const QString &message, int timeout) {
    // don't show the message immediately
    lastMessage_ = message;
    lastTimeOut_ = timeout;
    if(!messageTimer_->isActive()) {
        messageTimer_->start();
    }
}

void StatusBar::reallyShowMessage() {
    if(lastTimeOut_ == 0) {
        // set the text on the label for it not to change when a menubar item is focused
        statusLabel_->setText(lastMessage_);
    }
    else {
        QStatusBar::showMessage(lastMessage_, lastTimeOut_);
    }
}

}
