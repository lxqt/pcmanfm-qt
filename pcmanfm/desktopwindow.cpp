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

#include "./application.h"
#include "mainwindow.h"
#include "desktopitemdelegate.h"
#include "foldermenu.h"
#include "filemenu.h"
#include "cachedfoldermodel.h"
#include "folderview_p.h"

#include <QX11Info> // requires Qt 4 or Qt 5.1
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <xcb/xcb.h>
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

using namespace PCManFM;

DesktopWindow::DesktopWindow(int screenNum):
  View(Fm::FolderView::IconMode),
  screenNum_(screenNum),
  folder_(NULL),
  model_(NULL),
  proxyModel_(NULL),
  fileLauncher_(NULL),
  showWmMenu_(false),
  wallpaperMode_(WallpaperNone),
  relayoutTimer_(NULL) {

  QDesktopWidget* desktopWidget = QApplication::desktop();
  setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
  setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
  setAttribute(Qt::WA_DeleteOnClose);
  
  // set freedesktop.org EWMH hints properly
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  if(QX11Info::isPlatformX11() && QX11Info::connection()) {
    xcb_connection_t* con = QX11Info::connection();
    const char* atom_name = "_NET_WM_WINDOW_TYPE_DESKTOP";
    xcb_atom_t atom = xcb_intern_atom_reply(con, xcb_intern_atom(con, 0, strlen(atom_name), atom_name), NULL)->atom;
    const char* prop_atom_name = "_NET_WM_WINDOW_TYPE";
    xcb_atom_t prop_atom = xcb_intern_atom_reply(con, xcb_intern_atom(con, 0, strlen(prop_atom_name), prop_atom_name), NULL)->atom;
    xcb_atom_t XA_ATOM = 4;
    xcb_change_property(con, XCB_PROP_MODE_REPLACE, winId(), prop_atom, XA_ATOM, 32, 1, &atom);
  }
#else
  Atom atom = XInternAtom(QX11Info::display(), "_NET_WM_WINDOW_TYPE_DESKTOP", False);
  XChangeProperty(QX11Info::display(), winId(), XInternAtom(QX11Info::display(), "_NET_WM_WINDOW_TYPE", False), XA_ATOM, 32, PropModeReplace, (uchar*)&atom, 1);
#endif

  // paint background for the desktop widget
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Base);

  // set our custom file launcher
  View::setFileLauncher(&fileLauncher_);
  loadItemPositions();
  Settings& settings = static_cast<Application* >(qApp)->settings();
  
  model_ = Fm::CachedFolderModel::modelFromPath(fm_path_get_desktop());
  folder_ = reinterpret_cast<FmFolder*>(g_object_ref(model_->folder()));

  proxyModel_ = new Fm::ProxyFolderModel();
  proxyModel_->setSourceModel(model_);
  proxyModel_->setShowThumbnails(settings.showThumbnails());
  proxyModel_->sort(Fm::FolderModel::ColumnFileMTime);
  setModel(proxyModel_);

  listView_ = static_cast<Fm::FolderViewListView*>(childView());
  listView_->setMovement(QListView::Snap);
  listView_->setResizeMode(QListView::Adjust);
  listView_->setFlow(QListView::TopToBottom);

  // make the background of the list view transparent (alpha: 0)
  QPalette transparent = listView_->palette();
  transparent.setColor(QPalette::Base, QColor(0,0,0,0));
  listView_->setPalette(transparent);

  // set our own delegate
  delegate_ = new DesktopItemDelegate(listView_);
  listView_->setItemDelegateForColumn(Fm::FolderModel::ColumnFileName, delegate_);

  // remove frame
  listView_->setFrameShape(QFrame::NoFrame);

  // inhibit scrollbars FIXME: this should be optional in the future
  listView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  listView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  connect(proxyModel_, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(onRowsInserted(QModelIndex,int,int)));
  connect(proxyModel_, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), SLOT(onRowsAboutToBeRemoved(QModelIndex,int,int)));
  connect(proxyModel_, SIGNAL(layoutChanged()), SLOT(onLayoutChanged()));
  connect(listView_, SIGNAL(indexesMoved(QModelIndexList)), SLOT(onIndexesMoved(QModelIndexList)));

  connect(this, SIGNAL(openDirRequested(FmPath*, int)), SLOT(onOpenDirRequested(FmPath*, int)));
  
  listView_->installEventFilter(this);
}

DesktopWindow::~DesktopWindow() {
  listView_->removeEventFilter(this);

  if(relayoutTimer_)
    delete relayoutTimer_;

  if(proxyModel_)
    delete proxyModel_;

  if(model_)
    model_->unref();

  if(folder_)
    g_object_unref(folder_);
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
}

void DesktopWindow::setWallpaperFile(QString filename) {
  wallpaperFile_ = filename;
}

void DesktopWindow::setWallpaperMode(WallpaperMode mode) {
  wallpaperMode_ = mode;
}

QImage DesktopWindow::loadWallpaperFile(QSize requiredSize) {
  // see if we have a scaled version cached on disk
  QString cacheFileName = QString::fromLocal8Bit(qgetenv("XDG_CACHE_HOME"));
  if(cacheFileName.isEmpty())
    cacheFileName = QDir::homePath() % QLatin1String("/.cache");
  Application* app = static_cast<Application*>(qApp);
  cacheFileName += QLatin1String("/pcmanfm-qt/") % app->profileName();
  QDir().mkpath(cacheFileName); // ensure that the cache dir exists
  cacheFileName += QLatin1String("/wallpaper.cache");

  // read info file
  QString origin;
  QFile info(cacheFileName % ".info");
  if(info.open(QIODevice::ReadOnly)) {
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
  // we don't have a cached scaled image, load the original file
  QImage image(wallpaperFile_);
  qDebug() << "size of original image" << image.size();
  if(image.isNull() || image.size() == requiredSize) // if the original size is what we want
    return image;

  // scale the original image
  QImage scaled = image.scaled(requiredSize.width(), requiredSize.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  // FIXME: should we save the scaled image if its size is larger than the original image?

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
  setWallpaperFile(settings.wallpaper());
  setWallpaperMode(settings.wallpaperMode());
  setFont(settings.desktopFont());
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

  FmFileInfoList* files = menu->files();
  // select exactly one item
  if(fm_file_info_list_get_length(files) == 1) {
    FmFileInfo* file = menu->firstFile();
    if(customItemPos_.find(fm_file_info_get_name(file)) != customItemPos_.end()) {
      // the file item has a custom position
      action->setChecked(true);
    }
  }
  connect(action, SIGNAL(toggled(bool)), SLOT(onStickToCurrentPos(bool)));
}

void DesktopWindow::prepareFolderMenu(Fm::FolderMenu* menu) {
  PCManFM::View::prepareFolderMenu(menu);
  // remove file properties action
  menu->removeAction(menu->propertiesAction());
  // add an action for desktop preferences instead
  QAction* action = menu->addAction(tr("Desktop Preferences"));
  connect(action, SIGNAL(triggered(bool)), SLOT(onDesktopPreferences()));
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) // Qt 5

void DesktopWindow::xcbEvent(xcb_generic_event_t* generic_event) {
  int event_type = generic_event->response_type & ~0x80;
  if(event_type == XCB_PROPERTY_NOTIFY) {
    xcb_property_notify_event_t* event = reinterpret_cast<xcb_property_notify_event_t*>(generic_event);
    if(event->window == QX11Info::appRootWindow(screenNum_)) {
      // NOTE: the following is a workaround for Qt bug 32567.
      // https://bugreports.qt-project.org/browse/QTBUG-32567
      // Though the status of the bug report is closed, it's not yet fixed for X11.
      // In theory, QDesktopWidget should emit "workAreaResized()" signal when the work area
      // of any screen is changed, but in fact it does not do it.
      // Therefore we need to monitor the X11 event ourselves and do the update manually.
      static xcb_atom_t workarea_atom = 0;
      if(workarea_atom == 0) {
        const char* atom_name = "_NET_WORKAREA";
        xcb_connection_t* con = QX11Info::connection();
        workarea_atom = xcb_intern_atom_reply(con, xcb_intern_atom(con, 0, strlen(atom_name), atom_name), NULL)->atom;
      }
      if(event->atom == workarea_atom) { // work area is changed, upadte
        // When our event filter function is called, Qt has not handled the event yet.
        // So Qt still has the old work area size at the moment.
        // Schedule a timer to update the work area from Qt later.
        QTimer::singleShot(100, this, SLOT(updateWorkArea()));
      }
    }
  }
  else if(showWmMenu_) {
    // If we want to show the desktop menus provided by the window manager instead of ours,
    // we have to forward the mouse events we received to the root window.
    switch(event_type) {
      case XCB_BUTTON_PRESS: {
        xcb_button_press_event_t* event = reinterpret_cast<xcb_button_press_event_t*>(generic_event);
        if(event->event == effectiveWinId()) {
          // check if the user click on blank area
          QModelIndex index = listView_->indexAt(QPoint(event->event_x, event->event_y));
          if(!index.isValid()) {
            xcb_ungrab_pointer(QX11Info::connection(), event->time);
            // forward the event to the root window
            xcb_button_press_event_t event2 = *event;
            WId root = QX11Info::appRootWindow(QX11Info::appScreen());
            event2.event = root;
            xcb_send_event(QX11Info::connection(), 0, root, XCB_EVENT_MASK_BUTTON_PRESS, (char*)&event2);
          }
        }
        break;
      }
      case XCB_BUTTON_RELEASE: {
        xcb_button_release_event_t* event = reinterpret_cast<xcb_button_release_event_t*>(generic_event);
        if(event->event == effectiveWinId()) {
          // check if the user click on blank area
          QModelIndex index = listView_->indexAt(QPoint(event->event_x, event->event_y));
          if(!index.isValid()) {
            // forward the event to the root window
            xcb_button_release_event_t event2 = *event;
            WId root = QX11Info::appRootWindow(QX11Info::appScreen());
            event2.event = root;
            xcb_send_event(QX11Info::connection(), 0, root, XCB_EVENT_MASK_BUTTON_RELEASE, (char*)&event2);
          }
        }
        break;
      }
      default:
        break;
    }
  }
}

#else // Qt 4

bool DesktopWindow::x11Event(XEvent * event) {
  // FIXME: support Qt4
  if(showWmMenu_) {
    // filter all native X11 events (using Xlib)
    switch(event->type) {
      case ButtonPress: {
        if(event->xany.window == effectiveWinId()) {
          // check if the user click on blank area
          QModelIndex index = listView_->indexAt(QPoint(event->xbutton.x, event->xbutton.y));
          if(!index.isValid()) {
            XUngrabPointer(QX11Info::display(), event->xbutton.time);
            // forward the event to the root window
            XButtonEvent event2 = event->xbutton;
            WId root = QX11Info::appRootWindow(QX11Info::appScreen());
            event2.window = root;
            XSendEvent(QX11Info::display(), root, False, ButtonPressMask|ButtonReleaseMask, (XEvent*)&event2);
          }
        }
        break;
      }
      case ButtonRelease: {
        if(event->xany.window == effectiveWinId()) {
          // check if the user click on blank area
          QModelIndex index = listView_->indexAt(QPoint(event->xbutton.x, event->xbutton.y));
          if(!index.isValid()) {
            // forward the event to the root window
            XButtonEvent event2 = event->xbutton;
            WId root = QX11Info::appRootWindow(QX11Info::appScreen());
            event2.window = root;
            XSendEvent(QX11Info::display(), root, False, ButtonPressMask|ButtonReleaseMask, (XEvent*)&event2);
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }
  return QWidget::x11Event(event);
}

#endif

void DesktopWindow::updateWorkArea() {
  QRect rect = qApp->desktop()->availableGeometry(screenNum_);
  qDebug() << "new workarea" << rect;
  setWorkArea(rect);
}

void DesktopWindow::onDesktopPreferences() {
  static_cast<Application* >(qApp)->desktopPrefrences(QString());
}

void DesktopWindow::setWorkArea(const QRect& rect) {
  const QRect& windowRect = geometry();
  int left = rect.left() - windowRect.left();
  int top = rect.top() - windowRect.top();
  int right = windowRect.right() - rect.right();
  int bottom = windowRect.bottom() - rect.bottom();
  // We always set the size of the desktop window
  // to the size of the whole screen regardless of work area.
  // For work area, we reserve the margins on the list view
  // using style sheet instead. So icons are all painted
  // inside the work area but the background image still
  // covers the whole screen.
  layout()->setContentsMargins(left, top, right, bottom);
  if(!customItemPos_.isEmpty())
    queueRelayout();
}

void DesktopWindow::onRowsInserted(const QModelIndex& parent, int start, int end) {
  if(!customItemPos_.isEmpty())
    queueRelayout();
}

void DesktopWindow::onRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
  if(!customItemPos_.isEmpty()) {
    // also delete stored custom item positions for the items currently being removed.
    bool changed = false;
    for(int row = start; row <= end ;++row) {
      QModelIndex index = parent.child(row, 0);
      FmFileInfo* file = proxyModel_->fileInfoFromIndex(index);
      if(file) { // remove custom position for the item
        if(customItemPos_.remove(fm_file_info_get_name(file)))
          changed = true;
      }
    }
    if(changed)
      saveItemPositions();
    queueRelayout();
  }
}

void DesktopWindow::onLayoutChanged() {
  if(!customItemPos_.isEmpty())
    queueRelayout();
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
      FmFileInfo* file = proxyModel_->fileInfoFromIndex(index);
      QRect itemRect = listView_->rectForIndex(index);
      QByteArray name = fm_file_info_get_name(file);
      customItemPos_[name] = itemRect.topLeft();
      // qDebug() << "indexMoved:" << name << index << itemRect;
    }
  }
  saveItemPositions();
  queueRelayout();
}

// QListView does item layout in a very inflexible way, so let's do our custom layout again.
// FIXME: this is very inefficient, but due to the design flaw of QListView, this is currently the only workaround.
void DesktopWindow::relayoutItems() {
  // qDebug("relayoutItems()");
  if(relayoutTimer_) {
	// this slot might be called from the timer, so we cannot delete it directly here.
	relayoutTimer_->deleteLater();
	relayoutTimer_ = NULL;
  }
  // FIXME: we use an internal class declared in a private header here, which is pretty bad.
  QSize grid = listView_->gridSize();
  QPoint pos = listView_->contentsRect().topLeft();

  for(int row = 0, rowCount = proxyModel_->rowCount(); row < rowCount; ++row) {
    QModelIndex index = proxyModel_->index(row, 0);
    FmFileInfo* file = proxyModel_->fileInfoFromIndex(index);
    QByteArray name = fm_file_info_get_name(file);
    QHash<QByteArray, QPoint>::iterator it = customItemPos_.find(name);
    if(it != customItemPos_.end()) { // the item has a custom position
      QPoint customPos = *it;
      listView_->setPositionForIndex(customPos, index);
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
      listView_->setPositionForIndex(pos, index);
      // qDebug() << "set pos" << name << row << index << pos;
    }
    pos.setY(pos.y() + grid.height() + listView_->spacing());
    if(pos.y() + grid.height() > listView_->contentsRect().bottom()) {
      pos.setX(pos.x() + grid.width() + listView_->spacing());
      pos.setY(rect().top());
    }
  }
}

void DesktopWindow::loadItemPositions() {
  // load custom item positions
  Settings& settings = static_cast<Application*>(qApp)->settings();
  QString configFile = QString("%1/desktop-items-%2.conf").arg(settings.profileDir(settings.profileName())).arg(screenNum_);
  QSettings file(configFile, QSettings::IniFormat);
  Q_FOREACH(const QString& name, file.childGroups()) {
    file.beginGroup(name);
    QVariant var = file.value("pos");
    if(var.isValid())
      customItemPos_[name.toUtf8()] = var.toPoint();
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
    FmFileInfo* file = menu->firstFile();
    QByteArray name = fm_file_info_get_name(file);
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

void DesktopWindow::queueRelayout() {
  // qDebug() << "queueRelayout";
  if(!relayoutTimer_) {
    relayoutTimer_ = new QTimer();
    relayoutTimer_->setSingleShot(true);
    connect(relayoutTimer_, SIGNAL(timeout()), SLOT(relayoutItems()));
    relayoutTimer_->start();
  }
}
