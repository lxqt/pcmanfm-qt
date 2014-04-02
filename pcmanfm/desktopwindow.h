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


#ifndef PCMANFM_DESKTOPWINDOW_H
#define PCMANFM_DESKTOPWINDOW_H

#include "view.h"
#include "launcher.h"

namespace Fm {
  class CachedFolderModel;
  class ProxyFolderModel;
}

namespace PCManFM {

class DesktopItemDelegate;
class Settings;

class DesktopWindow : public View {
Q_OBJECT
public:
  enum WallpaperMode {
    WallpaperNone,
    WallpaperStretch,
    WallpaperFit,
    WallpaperCenter,
    WallpaperTile
  };

  explicit DesktopWindow();
  virtual ~DesktopWindow();

  void setForeground(const QColor& color);
  void setShadow(const QColor& color);
  void setBackground(const QColor& color);
  void setWallpaperFile(QString filename);
  void setWallpaperMode(WallpaperMode mode = WallpaperStretch);

  // void setWallpaperAlpha(qreal alpha);
  void updateWallpaper();
  void updateFromSettings(Settings& settings);

  void setWorkArea(const QRect& rect);
protected:
  virtual void prepareFolderMenu(Fm::FolderMenu* menu);
  virtual void prepareFileMenu(Fm::FileMenu* menu);
  virtual void resizeEvent(QResizeEvent* event);
  
protected Q_SLOTS:
  void onOpenDirRequested(FmPath* path, int target);
  void onDesktopPreferences();

private:
  Fm::ProxyFolderModel* proxyModel_;
  Fm::CachedFolderModel* model_;
  FmFolder* folder_;

  QColor fgColor_;
  QColor bgColor_;
  QColor shadowColor_;

  QString wallpaperFile_;
  WallpaperMode wallpaperMode_;
  QPixmap wallpaperPixmap_;
  DesktopItemDelegate* delegate_;
  Launcher fileLauncher_;
};

}

#endif // PCMANFM_DESKTOPWINDOW_H
