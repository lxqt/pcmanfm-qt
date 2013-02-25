/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

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


#include "desktopwindow.h"
#include <QWidget>
#include <QDesktopWidget>
#include "./application.h"
#include "mainwindow.h"

#include <QImage>
#include <QPixmap>
#include <QPalette>
#include <QBrush>

using namespace PCManFM;

DesktopWindow::DesktopWindow():
  View(Fm::FolderView::IconMode),
  folder(NULL),
  model(NULL),
  proxyModel(NULL) {

  QDesktopWidget* desktopWidget = QApplication::desktop();
  setWindowFlags(Qt::Window|Qt::FramelessWindowHint);
  setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);

  model = new Fm::FolderModel();
  proxyModel = new Fm::ProxyFolderModel();
  proxyModel->setSourceModel(model);
  folder = fm_folder_from_path(fm_path_get_desktop());
  model->setFolder(folder);
  setModel(proxyModel);

  QListView* listView = static_cast<QListView*>(childView());
  listView->setMovement(QListView::Snap);
  listView->setResizeMode(QListView::Adjust);
  listView->setFlow(QListView::TopToBottom);
  // setGridSize();

  // remove frame
  listView->setFrameShape(QFrame::NoFrame);
  
  // inhibit scrollbars FIXME: this should be optional in the future
  listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // set background
  // setBackground("/usr/share/backgrounds/A_Little_Quetzal_by_vgerasimov.jpg");
  connect(this, SIGNAL(openDirRequested(FmPath*,int)), SLOT(onOpenDirRequested(FmPath*,int)));
}

DesktopWindow::~DesktopWindow() {
  if(proxyModel)
    delete proxyModel;
  if(model)
    delete model;
  if(folder)
    g_object_unref(folder);
}

void DesktopWindow::setBackground(QPixmap pixmap) {
  QListView* listView = static_cast<QListView*>(childView());
  QPalette p(listView->palette());
  p.setBrush(QPalette::Base, pixmap);
  // p.setBrush(QPalette::Text, QColor(255, 255, 255));
  listView->setPalette(p);
}

void DesktopWindow::setBackground(QString fileName) {
  // QPixmap bg(fileName);
  QImage bg(fileName);
  setBackground(bg);
}

void DesktopWindow::setBackground(QImage image) {
  QListView* listView = static_cast<QListView*>(childView());
  QPalette p(listView->palette());
  p.setBrush(QPalette::Base, image);
  // p.setBrush(QPalette::Text, QColor(255, 255, 255));
  listView->setPalette(p);
}

void DesktopWindow::setForeground(QColor color) {
  QListView* listView = static_cast<QListView*>(childView());
  listView->palette().setBrush(QPalette::Text, color);
}

void DesktopWindow::setShadow(QColor color) {
  // TODO: implement drawing text with shadow
}

void DesktopWindow::onOpenDirRequested(FmPath* path, int target) {
  // open in new window unconditionally.
  Application* app = static_cast<Application*>(qApp);
  MainWindow* newWin = new MainWindow(path);
  // TODO: apply window size from app->settings
  newWin->resize(640, 480);
  newWin->show();
}


#include "desktopwindow.moc"
