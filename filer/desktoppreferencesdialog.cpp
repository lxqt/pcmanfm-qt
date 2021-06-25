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

#include "desktoppreferencesdialog.h"
#include "desktopwindow.h"
#include "settings.h"
#include "application.h"
#include "xdgdir.h"
#include <QFileDialog>
#include <QImageReader>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QRegExp>
#include <QDebug>
#include <QStandardPaths>

using namespace Filer;

DesktopPreferencesDialog::DesktopPreferencesDialog(QWidget* parent, Qt::WindowFlags f):
  QDialog(parent, f)
  {

  setAttribute(Qt::WA_DeleteOnClose);

  Settings& settings = static_cast<Application*>(qApp)->settings();
  static_cast<Application*>(qApp)->updateDesktopsFromSettings();

  ui.setupUi(this);

  // setup wallpaper modes
  connect(ui.wallpaperMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DesktopPreferencesDialog::onWallpaperModeChanged);
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

  connect(ui.browse, &QPushButton::clicked, this, &DesktopPreferencesDialog::onBrowseClicked);
  qDebug("wallpaper: %s", settings.wallpaper().toUtf8().data());
  ui.imageFile->setText(settings.wallpaper());

  ui.font->setFont(settings.desktopFont());

  ui.backgroundColor->setColor(settings.desktopBgColor());
  ui.textColor->setColor(settings.desktopFgColor());
  ui.shadowColor->setColor(settings.desktopShadowColor());

  // probono: Apply immediately
  connect(ui.backgroundColor, &Fm::ColorButton::changed,
          this, &DesktopPreferencesDialog::applySettings);
  connect(ui.textColor, &Fm::ColorButton::changed,
          this, &DesktopPreferencesDialog::applySettings);
  connect(ui.shadowColor, &Fm::ColorButton::changed,
          this, &DesktopPreferencesDialog::applySettings);
  connect(ui.font, &Fm::FontButton::changed,
          this, &DesktopPreferencesDialog::applySettings);
}

DesktopPreferencesDialog::~DesktopPreferencesDialog() {
}

void DesktopPreferencesDialog::applySettings()
{
  if (ui.backgroundColor->color() == ui.textColor->color() and ui.textColor->color() == ui.shadowColor->color()) {
      // probnoo: This means that the settings have not been loaded at this point yet, and we should not save the changes
      return;
  }

  Settings& settings = static_cast<Application*>(qApp)->settings();

  settings.setWallpaper(ui.imageFile->text());
  int mode = ui.wallpaperMode->itemData(ui.wallpaperMode->currentIndex()).toInt();
  settings.setWallpaperMode(mode);
  settings.setDesktopFont(ui.font->font());
  settings.setDesktopBgColor(ui.backgroundColor->color());
  settings.setDesktopFgColor(ui.textColor->color());
  settings.setDesktopShadowColor(ui.shadowColor->color());

  settings.save();
  static_cast<Application*>(qApp)->updateDesktopsFromSettings();
}

void DesktopPreferencesDialog::accept() {
  applySettings();
  QDialog::accept();
}

void DesktopPreferencesDialog::onWallpaperModeChanged(int index) {
  int mode = ui.wallpaperMode->itemData(index).toInt();

  bool enable = (mode != DesktopWindow::WallpaperNone);
  ui.imageFile->setEnabled(enable);
  ui.browse->setEnabled(enable);
  applySettings();
}

void DesktopPreferencesDialog::onBrowseClicked() {
  QFileDialog dlg;
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  dlg.setFileMode(QFileDialog::ExistingFile);
  // compose a name fileter from QImageReader
  QString filter;
  filter.reserve(256);
  filter = tr("Image Files");
  filter += " (";
  QList<QByteArray> formats = QImageReader::supportedImageFormats();
  Q_FOREACH(QByteArray format, formats) {
    filter += "*.";
    filter += format.toLower();
    filter += ' ';
  }
  filter += ')';
  dlg.setNameFilter(filter);
  dlg.setNameFilterDetailsVisible(false);
  if(dlg.exec() == QDialog::Accepted) {
    QString filename;
    filename = dlg.selectedFiles().first();
    ui.imageFile->setText(filename);
    applySettings();
  }
}
