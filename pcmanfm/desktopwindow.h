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
#include <unordered_map>
#include <string>

#include <QHash>
#include <QPoint>
#include <QByteArray>
#include <xcb/xcb.h>
#include <libfm-qt/core/folder.h>

namespace Fm {
class CachedFolderModel;
class ProxyFolderModel;
class FolderViewListView;
}

namespace PCManFM {

class DesktopItemDelegate;
class Settings;

class DesktopWindow : public View {
    Q_OBJECT
public:
    friend class Application;

    enum WallpaperMode {
        WallpaperNone,
        WallpaperStretch,
        WallpaperFit,
        WallpaperCenter,
        WallpaperTile,
        WallpaperZoom
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
    virtual void prepareFolderMenu(Fm::FolderMenu* menu) override;
    virtual void prepareFileMenu(Fm::FileMenu* menu) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void onFileClicked(int type, const std::shared_ptr<const Fm::FileInfo>& fileInfo) override;

    void loadItemPositions();
    void saveItemPositions();

    QImage loadWallpaperFile(QSize requiredSize);

    virtual bool event(QEvent* event) override;
    virtual bool eventFilter(QObject* watched, QEvent* event) override;

    virtual void childDropEvent(QDropEvent* e) override;
    virtual void closeEvent(QCloseEvent* event) override;

protected Q_SLOTS:
    void onOpenDirRequested(const Fm::FilePath& path, int target);
    void onDesktopPreferences();

    void onRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
    void onRowsInserted(const QModelIndex& parent, int start, int end);
    void onLayoutChanged();
    void onModelSortFilterChanged();
    void onIndexesMoved(const QModelIndexList& indexes);
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    void relayoutItems();
    void onStickToCurrentPos(bool toggled);

    // void updateWorkArea();

    // file operations
    void onCutActivated();
    void onCopyActivated();
    void onPasteActivated();
    void onRenameActivated();
    void onDeleteActivated();
    void onFilePropertiesActivated();

private:
    void removeBottomGap();
    void paintBackground(QPaintEvent* event);
    static void alignToGrid(QPoint& pos, const QPoint& topLeft, const QSize& grid, const int spacing);

private:
    Fm::ProxyFolderModel* proxyModel_;
    Fm::CachedFolderModel* model_;
    std::shared_ptr<Fm::Folder> folder_;
    Fm::FolderViewListView* listView_;

    QColor fgColor_;
    QColor bgColor_;
    QColor shadowColor_;
    QString wallpaperFile_;
    WallpaperMode wallpaperMode_;
    QPixmap wallpaperPixmap_;
    DesktopItemDelegate* delegate_;
    Launcher fileLauncher_;
    bool showWmMenu_;

    int screenNum_;
    std::unordered_map<std::string, QPoint> customItemPos_;
    QHash<QModelIndex, QString> displayNames_; // only for desktop entries and shortcuts
    QTimer* relayoutTimer_;
};

}

#endif // PCMANFM_DESKTOPWINDOW_H
