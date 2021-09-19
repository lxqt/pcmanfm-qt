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
#include <QHash>
#include <QPoint>
#include <QByteArray>
#include <xcb/xcb.h>

class QMenuBar;

namespace Fm {
  class FolderModel;
  class ProxyFolderModel;
  class FolderViewListView;
}

namespace Filer {

class DesktopItemDelegate;
class DesktopMainWindow;
class Settings;

class DesktopWindow : public View {
Q_OBJECT
public:
  friend class Application;

  enum WallpaperMode {
    WallpaperNone,
    WallpaperTransparent,
    WallpaperStretch,
    WallpaperFit,
    WallpaperCenter,
    WallpaperTile
  };

  explicit DesktopWindow(int screenNum);
  virtual ~DesktopWindow();

  void setForeground(const QColor& color);
  void setShadow(const QColor& color);
  void setBackground(const QColor& color);
  void setDesktopFolder();
  void setWallpaperFile(QString filename);
  void setWallpaperMode(WallpaperMode mode = WallpaperStretch);

  // void setWallpaperAlpha(qreal alpha);
  void updateWallpaper();
  void updateFromSettings(Settings& settings);

  void queueRelayout(int delay = 0);

  int screenNum() const {
    return screenNum_;
  }

  void setScreenNum(int num);

protected:
  virtual void prepareFolderMenu(Fm::FolderMenu* menu);
  virtual void prepareFileMenu(Fm::FileMenu* menu);
  virtual void resizeEvent(QResizeEvent* event);
  virtual void onFileClicked(int type, FmFileInfo* fileInfo);

  void loadItemPositions();
  void saveItemPositions();

  QImage loadWallpaperFile(QSize requiredSize);

  virtual bool event(QEvent* event);
  virtual bool eventFilter(QObject * watched, QEvent * event);

  virtual void childDropEvent(QDropEvent* e);
  virtual void closeEvent(QCloseEvent *event);

protected Q_SLOTS:
  void onOpenFolder(QString folder);
  void onOpenFolderAndSelectItems(QString folder, QStringList items);
  void onOpenTrash();
  void onOpenDesktop();
  void onOpenDocuments();
  void onOpenDownloads();
  void onOpenHome();
  void onOpenDirRequested(FmPath* path, int target);
  void onDesktopPreferences();
  void onFilerPreferences();
  void onGoUp();
  void onOpenActivated(); // probono
  void onNewFolder();
  void onNewBlankFile();
  void onOpenTerminal();
  void onFindFiles();
  void onAbout();
  void onEditBookmarks();

  void onShowHidden(bool hidden);
  void onSortColumn(int column);
  void onSortOrder(Qt::SortOrder order);
  void onFolderFirst(bool first);
  void onCaseSensitive(Qt::CaseSensitivity sensitivity);
  void onReload();

  void updateMenu();

  void onRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
  void onRowsInserted(const QModelIndex& parent, int start, int end);
  void onLayoutChanged();
  void onIndexesMoved(const QModelIndexList& indexes);

  void relayoutItems();
  void onStickToCurrentPos(bool toggled);

  // void updateWorkArea();

  // file operations
  void onCutActivated();
  void onCopyActivated();
  void onPasteActivated();
  void onDuplicateActivated();
  void onEmptyTrashActivated();
  void onRenameActivated();
  void onDeleteActivated();
  void onDeleteWithoutTrashActivated();
  void onFilePropertiesActivated();

private:
  Fm::ProxyFolderModel* proxyModel_;
  Fm::FolderModel* model_;
  FmFolder* folder_;
  Fm::FolderViewListView* listView_;

  QColor fgColor_;
  QColor bgColor_;
  QColor shadowColor_;
  QString wallpaperFile_;
  WallpaperMode wallpaperMode_;
  DesktopItemDelegate* delegate_;
  Launcher fileLauncher_;

  int screenNum_;
  QHash<QByteArray, QPoint> customItemPos_;
  QTimer* relayoutTimer_;
  DesktopMainWindow* desktopMainWindow_;
};

}

#endif // PCMANFM_DESKTOPWINDOW_H
