/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  PCMan <email>

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


#include "desktoppreferencesdialog.h"
#include "desktopwindow.h"
#include "settings.h"
#include "application.h"

using namespace PCManFM;

DesktopPreferencesDialog::DesktopPreferencesDialog(QWidget* parent, Qt::WindowFlags f):
  QDialog(parent, f) {

  setAttribute(Qt::WA_DeleteOnClose);
  
  Settings& settings = static_cast<Application*>(qApp)->settings();
  ui.setupUi(this);

  // setup wallpaper modes
  connect(ui.wallpaperMode, SIGNAL(currentIndexChanged(int)), SLOT(onWallpaperModeChanged(int)));
  ui.wallpaperMode->addItem(tr("Fill with background color only"), DesktopWindow::WallpaperNone);
  ui.wallpaperMode->addItem(tr("Stretch to fill the entire screen"), DesktopWindow::WallpaperStretch);
  ui.wallpaperMode->addItem(tr("Stretch to fit the screen"), DesktopWindow::WallpaperFit);
  ui.wallpaperMode->addItem(tr("Center on the screen"), DesktopWindow::WallpaperCenter);
  ui.wallpaperMode->addItem(tr("Tile the image to fill the entire screen"), DesktopWindow::WallpaperTile);
  int i;
  switch(settings.wallpaperMode()) {
    case DesktopWindow::WallpaperNone:
      i = 0;
      break;
    case DesktopWindow::WallpaperStretch:
      i = 1;
      break;
    case DesktopWindow::WallpaperFit:
      i = 2;
      break;
    case DesktopWindow::WallpaperCenter:
      i = 3;
      break;
    case DesktopWindow::WallpaperTile:
      i = 4;
      break;
    default:
      i = 0;
  }
  ui.wallpaperMode->setCurrentIndex(i);

  ui.imageFIle->setText(settings.wallpaper());

  ui.backgroundColor->setColor(settings.desktopBgColor());
  ui.textColor->setColor(settings.desktopFgColor());
  ui.shadowColor->setColor(settings.desktopShadowColor());
}

DesktopPreferencesDialog::~DesktopPreferencesDialog() {

}

void DesktopPreferencesDialog::accept() {
  Settings& settings = static_cast<Application*>(qApp)->settings();
  
  settings.setWallpaper(ui.imageFIle->text());

  int mode = ui.wallpaperMode->itemData(ui.wallpaperMode->currentIndex()).toInt();
  settings.setWallpaperMode(mode);
  
  settings.setDesktopBgColor(ui.backgroundColor->color());
  settings.setDesktopFgColor(ui.textColor->color());
  settings.setDesktopShadowColor(ui.shadowColor->color());

  QDialog::accept();

  static_cast<Application*>(qApp)->updateDesktopsFromSettings();
}

void DesktopPreferencesDialog::onWallpaperModeChanged(int index) {
  int n = ui.wallpaperMode->count();
  int mode = ui.wallpaperMode->itemData(index).toInt();

  bool enable = (mode != DesktopWindow::WallpaperNone);
  ui.imageFIle->setEnabled(enable);
  ui.browse->setEnabled(enable);
}


#include "desktoppreferencesdialog.moc"
