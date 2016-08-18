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

#include "desktopwindow.h"
#include <QWidget>
#include <QDesktopWidget>
#include <QPainter>
#include <QImage>
#include <QImageReader>
#include <QFile>
#include <QPixmap>
#include <QPalette>
#include <QBrush>
#include <QLayout>
#include <QDebug>
#include <QTimer>
#include <QSettings>
#include <QStringBuilder>
#include <QDir>
#include <QShortcut>
#include <QDropEvent>
#include <QMimeData>

#include "./application.h"
#include "mainwindow.h"
#include "desktopitemdelegate.h"
#include <libfm-qt/foldermenu.h>
#include <libfm-qt/filemenu.h>
#include <libfm-qt/cachedfoldermodel.h>
#include <libfm-qt/folderview_p.h>
#include <libfm-qt/fileoperation.h>
#include <libfm-qt/filepropsdialog.h>
#include <libfm-qt/utilities.h>
#include <libfm-qt/path.h>
#include <libfm-qt/fileinfo.h>
#include "xdgdir.h"

#include <QX11Info>
#include <QScreen>
#include <xcb/xcb.h>
#include <X11/Xlib.h>

namespace PCManFM {

DesktopWindow::DesktopWindow(int screenNum):
  View(Fm::FolderView::IconMode),
  proxyModel_(NULL),
  model_(NULL),
  wallpaperMode_(WallpaperNone),
  fileLauncher_(NULL),
  showWmMenu_(false),
  screenNum_(screenNum),
  relayoutTimer_(NULL) {

  QDesktopWidget* desktopWidget = QApplication::desktop();
  setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
  setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
  setAttribute(Qt::WA_DeleteOnClose);

  // set our custom file launcher
  View::setFileLauncher(&fileLauncher_);

  listView_ = static_cast<Fm::FolderViewListView*>(childView());
  listView_->setMovement(QListView::Snap);
  listView_->setResizeMode(QListView::Adjust);
  listView_->setFlow(QListView::TopToBottom);

  // NOTE: When XRnadR is in use, the all screens are actually combined to form a
  // large virtual desktop and only one DesktopWindow needs to be created and screenNum is -1.
  // In some older multihead setups, such as xinerama, every physical screen
  // is treated as a separate desktop so many instances of DesktopWindow may be created.
  // In this case we only want to show desktop icons on the primary screen.
  if(desktopWidget->isVirtualDesktop() || screenNum_ == desktopWidget->primaryScreen()) {
    loadItemPositions();
    Settings& settings = static_cast<Application* >(qApp)->settings();

    model_ = Fm::CachedFolderModel::modelFromPath(Fm::Path::getDesktop());
    folder_ = model_->folder();

    proxyModel_ = new Fm::ProxyFolderModel();
    proxyModel_->setSourceModel(model_);
    proxyModel_->setShowThumbnails(settings.showThumbnails());
    proxyModel_->sort(settings.desktopSortColumn(), settings.desktopSortOrder());
    proxyModel_->setFolderFirst(settings.desktopSortFolderFirst());
    setModel(proxyModel_);

    connect(proxyModel_, &Fm::ProxyFolderModel::rowsInserted, this, &DesktopWindow::onRowsInserted);
    connect(proxyModel_, &Fm::ProxyFolderModel::rowsAboutToBeRemoved, this, &DesktopWindow::onRowsAboutToBeRemoved);
    connect(proxyModel_, &Fm::ProxyFolderModel::layoutChanged, this, &DesktopWindow::onLayoutChanged);
    connect(proxyModel_, &Fm::ProxyFolderModel::sortFilterChanged, this, &DesktopWindow::onModelSortFilterChanged);
    connect(listView_, &QListView::indexesMoved, this, &DesktopWindow::onIndexesMoved);
  }

  // set our own delegate
  delegate_ = new DesktopItemDelegate(listView_);
  listView_->setItemDelegateForColumn(Fm::FolderModel::ColumnFileName, delegate_);

  // remove frame
  listView_->setFrameShape(QFrame::NoFrame);
  // inhibit scrollbars FIXME: this should be optional in the future
  listView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  listView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  connect(this, &DesktopWindow::openDirRequested, this, &DesktopWindow::onOpenDirRequested);

  listView_->installEventFilter(this);

  // setup shortcuts
  QShortcut* shortcut;
  shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_X), this); // cut
  connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onCutActivated);

  shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_C), this); // copy
  connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onCopyActivated);

  shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_V), this); // paste
  connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onPasteActivated);

  shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this); // select all
  connect(shortcut, &QShortcut::activated, listView_, &QListView::selectAll);

  shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this); // delete
  connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onDeleteActivated);

  shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this); // rename
  connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onRenameActivated);

  shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Return), this); // rename
  connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onFilePropertiesActivated);

  shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), this); // force delete
  connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onDeleteActivated);
}

DesktopWindow::~DesktopWindow() {
  listView_->removeEventFilter(this);

  if(relayoutTimer_)
    delete relayoutTimer_;

  if(proxyModel_)
    delete proxyModel_;

  if(model_)
    model_->unref();
}

void DesktopWindow::setBackground(const QColor& color) {
  bgColor_ = color;
}

void DesktopWindow::setForeground(const QColor& color) {
  QPalette p = listView_->palette();
  p.setBrush(QPalette::Text, color);
  listView_->setPalette(p);
  fgColor_ = color;
}

void DesktopWindow::setShadow(const QColor& color) {
  shadowColor_ = color;
  delegate_->setShadowColor(color);
}

void DesktopWindow::onOpenDirRequested(FmPath* path, int target) {
  // open in new window unconditionally.
  Application* app = static_cast<Application*>(qApp);
  MainWindow* newWin = new MainWindow(path);
  // apply window size from app->settings
  newWin->resize(app->settings().windowWidth(), app->settings().windowHeight());
  newWin->show();
}

void DesktopWindow::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);

  // resize wall paper if needed
  if(isVisible() && wallpaperMode_ != WallpaperNone && wallpaperMode_ != WallpaperTile) {
    updateWallpaper();
    update();
  }
  queueRelayout(100); // Qt use a 100 msec delay for relayout internally so we use it, too.
}

void DesktopWindow::setDesktopFolder() {
  Fm::Path path = Fm::Path::newForPath(XdgDir::readDesktopDir().toStdString().c_str());
  model_ = Fm::CachedFolderModel::modelFromPath(path);
  proxyModel_->setSourceModel(model_);
}

void DesktopWindow::setWallpaperFile(QString filename) {
  wallpaperFile_ = filename;
}

void DesktopWindow::setWallpaperMode(WallpaperMode mode) {
  wallpaperMode_ = mode;
}

QImage DesktopWindow::loadWallpaperFile(QSize requiredSize) {
  // NOTE: for ease of programming, we only use the cache for the primary screen.
  bool useCache = (screenNum_ == -1 || screenNum_ == 0);
  QFile info;
  QString cacheFileName;
  if(useCache) {
    // see if we have a scaled version cached on disk
    cacheFileName = QString::fromLocal8Bit(qgetenv("XDG_CACHE_HOME"));
    if(cacheFileName.isEmpty())
      cacheFileName = QDir::homePath() % QLatin1String("/.cache");
    Application* app = static_cast<Application*>(qApp);
    cacheFileName += QLatin1String("/pcmanfm-qt/") % app->profileName();
    QDir().mkpath(cacheFileName); // ensure that the cache dir exists
    cacheFileName += QLatin1String("/wallpaper.cache");

    // read info file
    QString origin;
    info.setFileName(cacheFileName % ".info");
    if(info.open(QIODevice::ReadOnly)) {
      // FIXME: we need to compare mtime to see if the cache is out of date
      origin = QString::fromLocal8Bit(info.readLine());
      info.close();
      if(!origin.isEmpty()) {
        // try to see if we can get the size of the cached image.
        QImageReader reader(cacheFileName);
        reader.setAutoDetectImageFormat(true);
        QSize cachedSize = reader.size();
        qDebug() << "size of cached file" << cachedSize << ", requiredSize:" << requiredSize;
        if(cachedSize.isValid()) {
          if(cachedSize == requiredSize) { // see if the cached wallpaper has the size we want
            QImage image = reader.read(); // return the loaded image
            qDebug() << "origin" << origin;
            if(origin == wallpaperFile_)
              return image;
          }
        }
      }
    }
    qDebug() << "no cached wallpaper. generate a new one!";
  }

  // we don't have a cached scaled image, load the original file
  QImage image(wallpaperFile_);
  qDebug() << "size of original image" << image.size();
  if(image.isNull() || image.size() == requiredSize) // if the original size is what we want
    return image;

  // scale the original image
  QImage scaled = image.scaled(requiredSize.width(), requiredSize.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  // FIXME: should we save the scaled image if its size is larger than the original image?

  if(useCache) {
    // write the path of the original image to the .info file
    if(info.open(QIODevice::WriteOnly)) {
      info.write(wallpaperFile_.toLocal8Bit());
      info.close();

      // write the scaled cache image to disk
      const char* format; // we keep jpg format for *.jpg files, and use png format for others.
      if(wallpaperFile_.endsWith(QLatin1String(".jpg"), Qt::CaseInsensitive) || wallpaperFile_.endsWith(QLatin1String(".jpeg"), Qt::CaseInsensitive))
        format = "JPG";
      else
        format = "PNG";
      scaled.save(cacheFileName, format);
    }
    qDebug() << "wallpaper cached saved to " << cacheFileName;
    // FIXME: we might delay the write of the cached image?
  }
  return scaled;
}

// really generate the background pixmap according to current settings and apply it.
void DesktopWindow::updateWallpaper() {
  // reset the brush
  // QPalette palette(listView_->palette());
  QPalette palette(Fm::FolderView::palette());

  if(wallpaperMode_ == WallpaperNone) { // use background color only
    palette.setBrush(QPalette::Base, bgColor_);
  }
  else { // use wallpaper
    QPixmap pixmap;
    QImage image;
    if(wallpaperMode_ == WallpaperTile) { // use the original size
      image = QImage(wallpaperFile_);
      pixmap = QPixmap::fromImage(image);
    }
    else if(wallpaperMode_ == WallpaperStretch) {
      image = loadWallpaperFile(size());
      pixmap = QPixmap::fromImage(image);
    }
    else { // WallpaperCenter || WallpaperFit
      if(wallpaperMode_ == WallpaperCenter) {
        image = QImage(wallpaperFile_); // load original image
      }
      else if(wallpaperMode_ == WallpaperFit) {
        // calculate the desired size
        QSize origSize = QImageReader(wallpaperFile_).size(); // get the size of the original file
        if(origSize.isValid()) {
          QSize desiredSize = origSize;
          desiredSize.scale(width(), height(), Qt::KeepAspectRatio);
          image = loadWallpaperFile(desiredSize); // load the scaled image
        }
      }
      if(!image.isNull()) {
        pixmap = QPixmap(size());
        QPainter painter(&pixmap);
        pixmap.fill(bgColor_);
        int x = (width() - image.width()) / 2;
        int y = (height() - image.height()) / 2;
        painter.drawImage(x, y, image);
      }
    }
    wallpaperPixmap_ = pixmap;
    if(!pixmap.isNull()) {
      QBrush brush(pixmap);
      palette.setBrush(QPalette::Base, brush);
    }
    else // if the image is not loaded, fallback to use background color only
      palette.setBrush(QPalette::Base, bgColor_);
  }

  //FIXME: we should set the pixmap to X11 root window?
  setPalette(palette);
}

void DesktopWindow::updateFromSettings(Settings& settings) {
  setDesktopFolder();
  setWallpaperFile(settings.wallpaper());
  setWallpaperMode(settings.wallpaperMode());
  setFont(settings.desktopFont());
  setIconSize(Fm::FolderView::IconMode, QSize(settings.desktopIconSize(), settings.desktopIconSize()));
  setMargins(settings.desktopCellMargins());
  // setIconSize and setMargins may trigger relayout of items by QListView, so we need to do the layout again.
  queueRelayout();
  setForeground(settings.desktopFgColor());
  setBackground(settings.desktopBgColor());
  setShadow(settings.desktopShadowColor());
  showWmMenu_ = settings.showWmMenu();
  updateWallpaper();
  update();
}

void DesktopWindow::onFileClicked(int type, FmFileInfo* fileInfo) {
  if(!fileInfo && showWmMenu_)
    return; // do not show the popup if we want to use the desktop menu provided by the WM.
  View::onFileClicked(type, fileInfo);
}

void DesktopWindow::prepareFileMenu(Fm::FileMenu* menu) {
  // qDebug("DesktopWindow::prepareFileMenu");
  PCManFM::View::prepareFileMenu(menu);
  QAction* action = new QAction(tr("Stic&k to Current Position"), menu);
  action->setCheckable(true);
  menu->insertSeparator(menu->separator2());
  menu->insertAction(menu->separator2(), action);

  Fm::FileInfoList files = menu->files();
  // select exactly one item
  if(fm_file_info_list_get_length(files) == 1) {
    Fm::FileInfo file = menu->firstFile();
    if(customItemPos_.find(file.getName()) != customItemPos_.end()) {
      // the file item has a custom position
      action->setChecked(true);
    }
  }
  connect(action, &QAction::toggled, this, &DesktopWindow::onStickToCurrentPos);
}

void DesktopWindow::prepareFolderMenu(Fm::FolderMenu* menu) {
  PCManFM::View::prepareFolderMenu(menu);
  // remove file properties action
  menu->removeAction(menu->propertiesAction());
  // add an action for desktop preferences instead
  QAction* action = menu->addAction(tr("Desktop Preferences"));
  connect(action, &QAction::triggered, this, &DesktopWindow::onDesktopPreferences);
}

void DesktopWindow::onDesktopPreferences() {
  static_cast<Application* >(qApp)->desktopPrefrences(QString());
}

void DesktopWindow::onRowsInserted(const QModelIndex& parent, int start, int end) {
  queueRelayout();
}

void DesktopWindow::onRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
  Q_UNUSED(parent);
  Q_UNUSED(start);
  Q_UNUSED(end);
  if(!customItemPos_.isEmpty()) {
    // also delete stored custom item positions for the items currently being removed.
    // Here we can't rely on ProxyFolderModel::fileInfoFromIndex() because, although rows
    // aren't removed yet, files are already removed.
    QHash<QByteArray, QPoint> _customItemPos = customItemPos_;
    char* dektopPath = Fm::Path::getDesktop().toStr();
    QString desktopDir = QString(dektopPath) + QString("/");
    g_free(dektopPath);
    QHash<QByteArray, QPoint>::iterator it;
    for(it = _customItemPos.begin(); it != _customItemPos.end(); ++it) {
      const QByteArray& name = it.key();
      if(!QFile::exists(desktopDir + QString::fromUtf8(name, name.length())))
        customItemPos_.remove(it.key());
    }
    if(customItemPos_ != _customItemPos)
      saveItemPositions();
  }
  queueRelayout();
}

void DesktopWindow::onLayoutChanged() {
  queueRelayout();
}

void DesktopWindow::onModelSortFilterChanged() {
  Settings& settings = static_cast<Application*>(qApp)->settings();
  settings.setDesktopSortColumn(static_cast<Fm::FolderModel::ColumnId>(proxyModel_->sortColumn()));
  settings.setDesktopSortOrder(proxyModel_->sortOrder());
  settings.setSesktopSortFolderFirst(proxyModel_->folderFirst());
}

void DesktopWindow::onIndexesMoved(const QModelIndexList& indexes) {
  // remember the custom position for the items
  Q_FOREACH(const QModelIndex& index, indexes) {
    // Under some circumstances, Qt might emit indexMoved for
    // every single cells in the same row. (when QAbstractItemView::SelectItems is set)
    // So indexes list may contain several indixes for the same row.
    // Since we only care about rows, not individual cells,
    // let's handle column 0 of every row here.
    if(index.column() == 0) {
      Fm::FileInfo file = proxyModel_->fileInfoFromIndex(index);
      QRect itemRect = listView_->rectForIndex(index);
      QPoint tl = itemRect.topLeft();
      QRect workArea = qApp->desktop()->availableGeometry(screenNum_);
      workArea.adjust(12, 12, -12, -12);
      if(customItemPos_.keys(tl).isEmpty() // don't put items on each other
         && tl.x() >= workArea.x() && tl.y() >= workArea.y()
         && tl.x() + listView_->gridSize().width() <= workArea.right() + 1 // for historical reasons (-> Qt doc)
         && tl.y() + listView_->gridSize().height() <= workArea.bottom() + 1) { // as above
        QByteArray name = file.getName();
        customItemPos_[name] = tl;
        // qDebug() << "indexMoved:" << name << index << itemRect;
      }
    }
  }
  saveItemPositions();
  queueRelayout();
}

void DesktopWindow::removeBottomGap() {
  /************************************************************
   NOTE: Desktop is an area bounded from below while icons snap
   to its grid srarting from above. Therefore, we try to adjust
   the vertical cell margin to prevent relatively large gaps
   from taking shape at the desktop bottom.
   ************************************************************/
  QSize cellMargins = getMargins();
  int workAreaHeight = qApp->desktop()->availableGeometry(screenNum_).height()
                       - 24; // a 12-pix margin will be considered everywhere
  int cellHeight = listView_->gridSize().height() + listView_->spacing();
  int iconNumber = workAreaHeight / cellHeight;
  int bottomGap = workAreaHeight % cellHeight;
  /*******************************************
   First try to make room for an extra icon...
   *******************************************/
  // If one pixel is subtracted from the vertical margin, cellHeight
  // will decrease by 2 while bottomGap will increase by 2*iconNumber.
  // So, we can add an icon to the bottom once this inequality holds:
  // bottomGap + 2*n*iconNumber >= cellHeight - 2*n
  // From here, we get our "subtrahend":
  qreal exactNumber = ((qreal)cellHeight - (qreal)bottomGap)
                      / (2.0 * (qreal)iconNumber + 2.0);
  int subtrahend = (int)exactNumber + ((int)exactNumber == exactNumber ? 0 : 1);
  Settings& settings = static_cast<Application*>(qApp)->settings();
  int minCellHeight = settings.desktopCellMargins().height();
  if(subtrahend > 0
     && cellMargins.height() - subtrahend >= minCellHeight) {
    cellMargins -= QSize(0, subtrahend);
  }
  /***************************************************
   ... but if that can't be done, try to spread icons!
   ***************************************************/
  else
    cellMargins += QSize(0, (bottomGap / iconNumber) / 2);
  // set the new margins (if they're changed)
  delegate_->setMargins(cellMargins);
  setMargins(cellMargins);
  // in case the text shadow is reset to (0,0,0,0)
  setShadow(settings.desktopShadowColor());
}

// QListView does item layout in a very inflexible way, so let's do our custom layout again.
// FIXME: this is very inefficient, but due to the design flaw of QListView, this is currently the only workaround.
void DesktopWindow::relayoutItems() {
  loadItemPositions(); // something may have changed
  // qDebug("relayoutItems()");
  if(relayoutTimer_) {
    // this slot might be called from the timer, so we cannot delete it directly here.
    relayoutTimer_->deleteLater();
    relayoutTimer_ = NULL;
  }

  QDesktopWidget* desktop = qApp->desktop();
  int screen = 0;
  int row = 0;
  int rowCount = proxyModel_->rowCount();
  for(;;) {
    if(desktop->isVirtualDesktop()) {
      if(screen >= desktop->numScreens())
        break;
    }else {
      screen = screenNum_;
    }
    QRect workArea = desktop->availableGeometry(screen);
    workArea.adjust(12, 12, -12, -12); // add a 12 pixel margin to the work area
    // qDebug() << "workArea" << screen <<  workArea;
    // FIXME: we use an internal class declared in a private header here, which is pretty bad.
    QSize grid = listView_->gridSize();
    QPoint pos = workArea.topLeft();
    for(; row < rowCount; ++row) {
      QModelIndex index = proxyModel_->index(row, 0);
      int itemWidth = delegate_->sizeHint(listView_->getViewOptions(), index).width();
      Fm::FileInfo file = proxyModel_->fileInfoFromIndex(index);
      QByteArray name = file.getName();
      QHash<QByteArray, QPoint>::iterator it = customItemPos_.find(name);
      if(it != customItemPos_.end()) { // the item has a custom position
        QPoint customPos = *it;
        // center the contents vertically
        listView_->setPositionForIndex(customPos + QPoint((grid.width() - itemWidth) / 2, 0), index);
        // qDebug() << "set custom pos:" << name << row << index << customPos;
        continue;
      }
      // check if the current pos is alredy occupied by a custom item
      bool used = false;
      for(it = customItemPos_.begin(); it != customItemPos_.end(); ++it) {
        QPoint customPos = *it;
        if(QRect(customPos, grid).contains(pos)) {
          used = true;
          break;
        }
      }
      if(used) { // go to next pos
        --row;
      }
      else {
        // center the contents vertically
        listView_->setPositionForIndex(pos + QPoint((grid.width() - itemWidth) / 2, 0), index);
        // qDebug() << "set pos" << name << row << index << pos;
      }
      // move to next cell in the column
      pos.setY(pos.y() + grid.height() + listView_->spacing());
      if(pos.y() + grid.height() > workArea.bottom() + 1) {
        // if the next position may exceed the bottom of work area, go to the top of next column
        pos.setX(pos.x() + grid.width() + listView_->spacing());
        pos.setY(workArea.top());

        // check if the new column exceeds the right margin of work area
        if(pos.x() + grid.width() > workArea.right() + 1) {
          if(desktop->isVirtualDesktop()) {
            // in virtual desktop mode, go to next screen
            ++screen;
            break;
          }
        }
      }
    }
    if(row >= rowCount)
      break;
  }
}

void DesktopWindow::loadItemPositions() {
  // load custom item positions
  customItemPos_.clear();
  Settings& settings = static_cast<Application*>(qApp)->settings();
  QString configFile = QString("%1/desktop-items-%2.conf").arg(settings.profileDir(settings.profileName())).arg(screenNum_);
  QSettings file(configFile, QSettings::IniFormat);
  QSize grid = listView_->gridSize();
  QRect workArea = qApp->desktop()->availableGeometry(screenNum_);
  workArea.adjust(12, 12, -12, -12);
  char* dektopPath = Fm::Path::getDesktop().toStr();
  QString desktopDir = QString(dektopPath) + QString("/");
  g_free(dektopPath);
  Q_FOREACH(const QString& name, file.childGroups()) {
    if(!QFile::exists(desktopDir + name.toUtf8())) {
      // the file may have been removed from outside LXQT
      continue;
    }
    file.beginGroup(name);
    QVariant var = file.value("pos");
    if(var.isValid()) {
      QPoint customPos = var.toPoint();
      if (customPos.x() >= workArea.x() && customPos.y() >= workArea.y()
          && customPos.x() + listView_->gridSize().width() <= workArea.right() + 1
          && customPos.y() + listView_->gridSize().height() <= workArea.bottom() + 1)
      {
        // correct positions that are't aligned to the grid
        qreal w = qAbs((qreal)customPos.x() - (qreal)workArea.x())
                  / (qreal)(grid.width() + listView_->spacing());
        qreal h = qAbs(customPos.y() - (qreal)workArea.y())
                  / (qreal)(grid.height() + listView_->spacing());
        customPos.setX(workArea.x() + qRound(w) * (grid.width() + listView_->spacing()));
        customPos.setY(workArea.y() + qRound(h) * (grid.height() + listView_->spacing()));
        while(customItemPos_.values().contains(customPos)) {
          customPos.setY(customPos.y() + grid.height() + listView_->spacing());
          if(customPos.y() + grid.height() > workArea.bottom() + 1) {
            customPos.setX(customPos.x() + grid.width() + listView_->spacing());
            customPos.setY(workArea.top());
          }
        }
        customItemPos_[name.toUtf8()] = customPos;
      }
    }
    file.endGroup();
  }
}

void DesktopWindow::saveItemPositions() {
  Settings& settings = static_cast<Application*>(qApp)->settings();
  // store custom item positions
  QString configFile = QString("%1/desktop-items-%2.conf").arg(settings.profileDir(settings.profileName())).arg(screenNum_);
  // FIXME: using QSettings here is inefficient and it's not friendly to UTF-8.
  QSettings file(configFile, QSettings::IniFormat);
  file.clear(); // remove all existing entries

  // FIXME: we have to remove dead entries not associated to any files?
  QHash<QByteArray, QPoint>::iterator it;
  for(it = customItemPos_.begin(); it != customItemPos_.end(); ++it) {
    const QByteArray& name = it.key();
    QPoint pos = it.value();
    file.beginGroup(QString::fromUtf8(name, name.length()));
    file.setValue("pos", pos);
    file.endGroup();
  }
}

void DesktopWindow::onStickToCurrentPos(bool toggled) {
  QAction* action = static_cast<QAction*>(sender());
  Fm::FileMenu* menu = static_cast<Fm::FileMenu*>(action->parent());

  QModelIndexList indexes = listView_->selectionModel()->selectedIndexes();
  if(!indexes.isEmpty()) {
    Fm::FileInfo file = menu->firstFile();
    QByteArray name = file.getName();
    QModelIndex index = indexes.first();
    if(toggled) { // remember to current custom position
      QRect itemRect = listView_->rectForIndex(index);
      customItemPos_[name] = itemRect.topLeft();
      saveItemPositions();
    }
    else { // cancel custom position and perform relayout
      QHash<QByteArray, QPoint>::iterator it = customItemPos_.find(name);
      if(it != customItemPos_.end()) {
        customItemPos_.erase(it);
        saveItemPositions();
        relayoutItems();
      }
    }
  }
}

void DesktopWindow::queueRelayout(int delay) {
  // qDebug() << "queueRelayout";
  removeBottomGap();
  if(!relayoutTimer_) {
    relayoutTimer_ = new QTimer();
    relayoutTimer_->setSingleShot(true);
    connect(relayoutTimer_, &QTimer::timeout, this, &DesktopWindow::relayoutItems);
    relayoutTimer_->start(delay);
  }
}

// slots for file operations

void DesktopWindow::onCutActivated() {
  Fm::PathList paths = selectedFilePaths();
  if(!paths.isNull()) {
    Fm::cutFilesToClipboard(paths);
  }
}

void DesktopWindow::onCopyActivated() {
  Fm::PathList paths = selectedFilePaths();
  if(!paths.isNull()) {
    Fm::copyFilesToClipboard(paths);
  }
}

void DesktopWindow::onPasteActivated() {
  Fm::pasteFilesFromClipboard(path());
}

void DesktopWindow::onDeleteActivated() {
  Fm::PathList paths = selectedFilePaths();
  if(!paths.isNull()) {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    bool shiftPressed = (qApp->keyboardModifiers() & Qt::ShiftModifier ? true : false);
    if(settings.useTrash() && !shiftPressed)
      Fm::FileOperation::trashFiles(paths, settings.confirmTrash());
    else
      Fm::FileOperation::deleteFiles(paths, settings.confirmDelete());
  }
}

void DesktopWindow::onRenameActivated() {
  Fm::FileInfoList files = selectedFiles();
  if(!files.isNull()) {
    for(GList* l = fm_file_info_list_peek_head_link(files); l; l = l->next) {
      FmFileInfo* info = FM_FILE_INFO(l->data);
      Fm::renameFile(info, NULL);
    }
  }
}

void DesktopWindow::onFilePropertiesActivated() {
  Fm::FileInfoList files = selectedFiles();
  if(!files.isNull()) {
    Fm::FilePropsDialog::showForFiles(files);
  }
}

static void forwardMouseEventToRoot(QMouseEvent* event) {
  xcb_ungrab_pointer(QX11Info::connection(), event->timestamp());
  // forward the event to the root window
  xcb_button_press_event_t xcb_event;
  uint32_t mask = 0;
  xcb_event.state = 0;
  switch(event->type()) {
  case QEvent::MouseButtonPress:
    xcb_event.response_type = XCB_BUTTON_PRESS;
    mask = XCB_EVENT_MASK_BUTTON_PRESS;
    break;
  case QEvent::MouseButtonRelease:
      xcb_event.response_type = XCB_BUTTON_RELEASE;
      mask = XCB_EVENT_MASK_BUTTON_RELEASE;
    break;
  default:
    return;
  }

  // convert Qt button to XCB button
  switch(event->button()) {
  case Qt::LeftButton:
    xcb_event.detail = 1;
    xcb_event.state |= XCB_BUTTON_MASK_1;
    break;
  case Qt::MiddleButton:
    xcb_event.detail = 2;
    xcb_event.state |= XCB_BUTTON_MASK_2;
    break;
  case Qt::RightButton:
    xcb_event.detail = 3;
    xcb_event.state |= XCB_BUTTON_MASK_3;
    break;
  default:
    xcb_event.detail = 0;
  }

  // convert Qt modifiers to XCB states
  if(event->modifiers() & Qt::ShiftModifier)
    xcb_event.state |= XCB_MOD_MASK_SHIFT;
  if(event->modifiers() & Qt::ControlModifier)
    xcb_event.state |= XCB_MOD_MASK_SHIFT;
  if(event->modifiers() & Qt::AltModifier)
    xcb_event.state |= XCB_MOD_MASK_1;

  xcb_event.sequence = 0;
  xcb_event.time = event->timestamp();

  WId root = QX11Info::appRootWindow(QX11Info::appScreen());
  xcb_event.event = root;
  xcb_event.root = root;
  xcb_event.child = 0;

  xcb_event.root_x = event->globalX();
  xcb_event.root_y = event->globalY();
  xcb_event.event_x = event->x();
  xcb_event.event_y = event->y();
  xcb_event.same_screen = 1;

  xcb_send_event(QX11Info::connection(), 0, root, mask, (char*)&xcb_event);
  xcb_flush(QX11Info::connection());
}

bool DesktopWindow::event(QEvent* event)
{
  switch(event->type()) {
  case QEvent::WinIdChange: {
    qDebug() << "winid change:" << effectiveWinId();
    if(effectiveWinId() == 0)
      break;
    // set freedesktop.org EWMH hints properly
    if(QX11Info::isPlatformX11() && QX11Info::connection()) {
      xcb_connection_t* con = QX11Info::connection();
      const char* atom_name = "_NET_WM_WINDOW_TYPE_DESKTOP";
      xcb_atom_t atom = xcb_intern_atom_reply(con, xcb_intern_atom(con, 0, strlen(atom_name), atom_name), NULL)->atom;
      const char* prop_atom_name = "_NET_WM_WINDOW_TYPE";
      xcb_atom_t prop_atom = xcb_intern_atom_reply(con, xcb_intern_atom(con, 0, strlen(prop_atom_name), prop_atom_name), NULL)->atom;
      xcb_atom_t XA_ATOM = 4;
      xcb_change_property(con, XCB_PROP_MODE_REPLACE, effectiveWinId(), prop_atom, XA_ATOM, 32, 1, &atom);
    }
    break;
  }
#undef FontChange // FontChange is defined in the headers of XLib and clashes with Qt, let's undefine it.
  case QEvent::StyleChange:
  case QEvent::FontChange:
    queueRelayout();
    break;

  default:
      break;
  }

  return QWidget::event(event);
}

#undef FontChange // this seems to be defined in Xlib headers as a macro, undef it!

bool DesktopWindow::eventFilter(QObject * watched, QEvent * event) {
  if(watched == listView_) {
    switch(event->type()) {
    case QEvent::StyleChange:
    case QEvent::FontChange:
      if(model_)
        queueRelayout();
      break;
    default:
      break;
    }
  }
  else if(watched == listView_->viewport()) {
    switch(event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        if(showWmMenu_) {
          QMouseEvent* e = static_cast<QMouseEvent*>(event);
          // If we want to show the desktop menus provided by the window manager instead of ours,
          // we have to forward the mouse events we received to the root window.
          // check if the user click on blank area
          QModelIndex index = listView_->indexAt(e->pos());
          if(!index.isValid() && e->button() != Qt::LeftButton) {
            forwardMouseEventToRoot(e);
          }
      }
      break;
    default:
      break;
    }
  }
  return false;
}

void DesktopWindow::childDropEvent(QDropEvent* e) {
  bool moveItem = false;
  if(e->source() == listView_ && e->keyboardModifiers() == Qt::NoModifier) {
    // drag source is our list view, and no other modifier keys are pressed
    // => we're dragging desktop items
    const QMimeData *mimeData = e->mimeData();
    if(mimeData->hasFormat("application/x-qabstractitemmodeldatalist")) {
      QModelIndex dropIndex = listView_->indexAt(e->pos());
      if(dropIndex.isValid()) { // drop on an item
        QModelIndexList selected = selectedIndexes(); // the dragged items
        if(selected.contains(dropIndex)) { // drop on self, ignore
          moveItem = true;
        }
      }
      else { // drop on a blank area
        moveItem = true;
      }
    }
  }
  if(moveItem)
    e->accept();
  else
    Fm::FolderView::childDropEvent(e);
}

void DesktopWindow::closeEvent(QCloseEvent *event) {
  // prevent the desktop window from being closed.
  event->ignore();
}

void DesktopWindow::setScreenNum(int num) {
  if(screenNum_ != num) {
    screenNum_ = num;
    queueRelayout();
  }
}

} // namespace PCManFM
