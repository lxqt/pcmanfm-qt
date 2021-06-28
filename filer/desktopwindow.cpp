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
#include <QMainWindow>
#include <QMenuBar>
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
#include "foldermenu.h"
#include "filemenu.h"
#include "foldermodel.h"
#include "folderview_p.h"
#include "fileoperation.h"
#include "filepropsdialog.h"
#include "utilities.h"
#include "path.h"
#include "xdgdir.h"
#include "desktopmainwindow.h"
#include "../libfm-qt/path.h"
#include "../libfm-qt/utilities.h"
#include "windowregistry.h"
#include "ui_about.h"
#include "trash.h"

#include <QX11Info>
#include <QScreen>
#include <QStandardPaths>
#include <xcb/xcb.h>
#include <X11/Xlib.h>

using namespace Filer;

DesktopWindow::DesktopWindow(int screenNum):
    View(Fm::FolderView::IconMode),
    screenNum_(screenNum),
    folder_(NULL),
    model_(NULL),
    proxyModel_(NULL),
    fileLauncher_(NULL),
    wallpaperMode_(WallpaperNone),
    relayoutTimer_(NULL),
    desktopMainWindow_(NULL){

    QDesktopWidget* desktopWidget = QApplication::desktop();
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
    setAttribute(Qt::WA_DeleteOnClose);

    // set our custom file launcher
    View::setFileLauncher(&fileLauncher_);

    listView_ = static_cast<Fm::FolderViewListView*>(childView());
    listView_->setMovement(QListView::Free);
    listView_->setResizeMode(QListView::Adjust);
    listView_->setFlow(QListView::TopToBottom);

    // give listView_ an object name so we can refer to it in stylesheets -
    // this is actually the widget that has the wallpaper background
    listView_->setObjectName("DesktopListView");

    // NOTE: When XRnadR is in use, the all screens are actually combined to form a
    // large virtual desktop and only one DesktopWindow needs to be created and screenNum is -1.
    // In some older multihead setups, such as xinerama, every physical screen
    // is treated as a separate desktop so many instances of DesktopWindow may be created.
    // In this case we only want to show desktop icons on the primary screen.
    if(desktopWidget->isVirtualDesktop() || screenNum_ == desktopWidget->primaryScreen()) {
        loadItemPositions();
        Settings& settings = static_cast<Application* >(qApp)->settings();

        FmFolder* folder = fm_folder_from_path(fm_path_get_desktop());
        model_ = new Fm::FolderModel();
        model_->setFolder(folder, true);
        folder_ = reinterpret_cast<FmFolder*>(g_object_ref(model_->folder()));

        proxyModel_ = new Fm::ProxyFolderModel();
        proxyModel_->setSourceModel(model_);
        proxyModel_->setShowThumbnails(settings.showThumbnails());
        proxyModel_->sort(Fm::FolderModel::ColumnFileMTime);
        proxyModel_->setDesktopMode();
        setModel(proxyModel_);

        connect(proxyModel_, &Fm::ProxyFolderModel::rowsInserted, this, &DesktopWindow::onRowsInserted);
        connect(proxyModel_, &Fm::ProxyFolderModel::rowsAboutToBeRemoved, this, &DesktopWindow::onRowsAboutToBeRemoved);
        connect(proxyModel_, &Fm::ProxyFolderModel::layoutChanged, this, &DesktopWindow::onLayoutChanged);
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

    shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), this); // pronono: open
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onOpenActivated); // probono

    /*
     * probono: Commenting these out
     * for those that are alraedy defined in the Menu solves QAction::event: Ambiguous shortcut overload

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

    */

    shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), this); // force delete
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onDeleteWithoutTrashActivated);

    shortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::CTRL + Qt::Key_Backspace), this); // probono: force delete
    connect(shortcut, &QShortcut::activated, this, &DesktopWindow::onDeleteWithoutTrashActivated);

    desktopMainWindow_ = new DesktopMainWindow(nullptr);
    layout()->setMenuBar(desktopMainWindow_->getMenuBar());

    updateMenu();

    connect(desktopMainWindow_, &DesktopMainWindow::open, this, &DesktopWindow::onOpenActivated);
    connect(desktopMainWindow_, &DesktopMainWindow::fileProperties, this, &DesktopWindow::onFilePropertiesActivated);
    connect(desktopMainWindow_, &DesktopMainWindow::preferences, this, &DesktopWindow::onFilerPreferences);
    connect(desktopMainWindow_, &DesktopMainWindow::openFolder, this, &DesktopWindow::onOpenFolder);
    connect(desktopMainWindow_, &DesktopMainWindow::openTrash, this, &DesktopWindow::onOpenTrash);
    connect(desktopMainWindow_, &DesktopMainWindow::openDesktop, this, &DesktopWindow::onOpenDesktop);
    connect(desktopMainWindow_, &DesktopMainWindow::openDocuments, this, &DesktopWindow::onOpenDocuments);
    connect(desktopMainWindow_, &DesktopMainWindow::openDownloads, this, &DesktopWindow::onOpenDownloads);
    connect(desktopMainWindow_, &DesktopMainWindow::openHome, this, &DesktopWindow::onOpenHome);
    connect(desktopMainWindow_, &DesktopMainWindow::goUp, this, &DesktopWindow::onGoUp);
    connect(desktopMainWindow_, &DesktopMainWindow::cut, this, &DesktopWindow::onCutActivated);
    connect(desktopMainWindow_, &DesktopMainWindow::copy, this, &DesktopWindow::onCopyActivated);
    connect(desktopMainWindow_, &DesktopMainWindow::paste, this, &DesktopWindow::onPasteActivated);
    connect(desktopMainWindow_, &DesktopMainWindow::duplicate, this, &DesktopWindow::onDuplicateActivated);
    connect(desktopMainWindow_, &DesktopMainWindow::emptyTrash, this, &DesktopWindow::onEmptyTrashActivated);
    connect(desktopMainWindow_, &DesktopMainWindow::del, this, &DesktopWindow::onDeleteActivated);
    connect(desktopMainWindow_, &DesktopMainWindow::rename, this, &DesktopWindow::onRenameActivated);
    connect(desktopMainWindow_, &DesktopMainWindow::selectAll, listView_, &QListView::selectAll);
    connect(desktopMainWindow_, &DesktopMainWindow::invert, this, &FolderView::invertSelection);
    connect(desktopMainWindow_, &DesktopMainWindow::newFolder, this, &DesktopWindow::onNewFolder);
    connect(desktopMainWindow_, &DesktopMainWindow::newBlankFile, this, &DesktopWindow::onNewBlankFile);
    connect(desktopMainWindow_, &DesktopMainWindow::openTerminal, this, &DesktopWindow::onOpenTerminal);
    connect(desktopMainWindow_, &DesktopMainWindow::search, this, &DesktopWindow::onFindFiles);
    connect(desktopMainWindow_, &DesktopMainWindow::about, this, &DesktopWindow::onAbout);
    connect(desktopMainWindow_, &DesktopMainWindow::editBookmarks, this, &DesktopWindow::onEditBookmarks);
    connect(desktopMainWindow_, &DesktopMainWindow::showHidden, this, &DesktopWindow::onShowHidden);
    connect(proxyModel_, &Fm::ProxyFolderModel::sortFilterChanged, this, &DesktopWindow::updateMenu);
    connect(desktopMainWindow_, &DesktopMainWindow::sortColumn, this, &DesktopWindow::onSortColumn);
    connect(desktopMainWindow_, &DesktopMainWindow::sortOrder, this, &DesktopWindow::onSortOrder);
    connect(desktopMainWindow_, &DesktopMainWindow::folderFirst, this, &DesktopWindow::onFolderFirst);
    connect(desktopMainWindow_, &DesktopMainWindow::caseSensitive, this, &DesktopWindow::onCaseSensitive);
    connect(desktopMainWindow_, &DesktopMainWindow::reload, this, &DesktopWindow::onReload);
}

DesktopWindow::~DesktopWindow() {
    listView_->removeEventFilter(this);

    if(relayoutTimer_)
        delete relayoutTimer_;

    if(proxyModel_)
        delete proxyModel_;

    if(model_)
        delete model_;

    if(folder_)
        g_object_unref(folder_);
}

void DesktopWindow::setBackground(const QColor& color) {
    bgColor_ = color;
}

void DesktopWindow::setForeground(const QColor& color) {
    fgColor_ = color;
    delegate_->setTextColor(color);
}

void DesktopWindow::setShadow(const QColor& color) {
    shadowColor_ = color;
    delegate_->setShadowColor(color);
}

void DesktopWindow::onOpenDirRequested(FmPath* path, int target) {

    // just raise the window if it's already open
    if (WindowRegistry::instance().checkPathAndRaise(fm_path_to_str(path))) {
      return;
    }

    Application* app = static_cast<Application*>(qApp);
    MainWindow* newWin = new MainWindow(path);
    // apply window size from app->settings
    if ( ! app->settings().spatialMode() ) {
      newWin->resize(app->settings().windowWidth(), app->settings().windowHeight());
      if(app->settings().windowMaximized()) {
              newWin->setWindowState(newWin->windowState() | Qt::WindowMaximized);
      }
    }
    newWin->show();
}

void DesktopWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // resize wall paper if needed
    if(isVisible() && wallpaperMode_ != WallpaperNone && wallpaperMode_ != WallpaperTransparent && wallpaperMode_ != WallpaperTile) {
        updateWallpaper();
        update();
    }
    queueRelayout(100); // Qt use a 100 msec delay for relayout internally so we use it, too.
}

void DesktopWindow::setDesktopFolder() {
    FmPath *path = fm_path_new_for_path(XdgDir::readDesktopDir().toStdString().c_str());
    FmFolder* folder = fm_folder_from_path(path);
    if (model_)
      delete model_;
    model_ = new Fm::FolderModel();
    model_->setFolder(folder, true);
    folder_ = reinterpret_cast<FmFolder*>(g_object_ref(model_->folder()));
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
        cacheFileName += QLatin1String("/filer-qt/") % app->profileName();
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
    setStyleSheet("");
    switch (wallpaperMode_) {
        case Filer::DesktopWindow::WallpaperNone:
          setStyleSheet("#DesktopListView { background-color: " + bgColor_.name() + " }");
          break;
    case Filer::DesktopWindow::WallpaperTransparent:
          setStyleSheet("background-color: transparent");
          break;
    case Filer::DesktopWindow::WallpaperStretch:
          if (! wallpaperFile_.isEmpty())
              setStyleSheet("#DesktopListView { border-image: url(" + wallpaperFile_ + ") stretch stretch }");
          break;
    case Filer::DesktopWindow::WallpaperFit: // FIXME - how do we fit with correct aspect in qt stylesheets?
          if (! wallpaperFile_.isEmpty())
              setStyleSheet("#DesktopListView { border-image: url(" + wallpaperFile_ + ") stretch }");
          break;
    case Filer::DesktopWindow::WallpaperCenter:
          if (! wallpaperFile_.isEmpty())
              setStyleSheet("#DesktopListView { background-image: url(" + wallpaperFile_
                            + "); background-repeat: no-repeat; background-position: center; background-color: "
                            + bgColor_.name() + " }");
          break;
    case Filer::DesktopWindow::WallpaperTile:
          if (! wallpaperFile_.isEmpty())
              setStyleSheet("#DesktopListView { background-image: url(" + wallpaperFile_ + ") }");
          break;
    }
}

void DesktopWindow::updateFromSettings(Settings& settings) {
    setDesktopFolder();
    setWallpaperFile(settings.wallpaper());
    setWallpaperMode(settings.wallpaperMode());
    setFont(settings.desktopFont());
    delegate_->setFont(settings.desktopFont());
    setIconSize(Fm::FolderView::IconMode, QSize(settings.bigIconSize(), settings.bigIconSize()));
    // setIconSize may trigger relayout of items by QListView, so we need to do the layout again.
    queueRelayout();
    setForeground(settings.desktopFgColor());
    setBackground(settings.desktopBgColor());
    setShadow(settings.desktopShadowColor());
    updateWallpaper();
    update();
}

void DesktopWindow::onFileClicked(int type, FmFileInfo* fileInfo) {
    View::onFileClicked(type, fileInfo);
}

void DesktopWindow::prepareFileMenu(Fm::FileMenu* menu) {
    // qDebug("DesktopWindow::prepareFileMenu");
    Filer::View::prepareFileMenu(menu);
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
    connect(action, &QAction::toggled, this, &DesktopWindow::onStickToCurrentPos);
}

void DesktopWindow::prepareFolderMenu(Fm::FolderMenu* menu) {
    Filer::View::prepareFolderMenu(menu);
    // remove file properties action
    menu->removeAction(menu->propertiesAction());
    // add an action for desktop preferences instead
    QAction* action = menu->addAction(tr("Desktop Preferences"));
    connect(action, &QAction::triggered, this, &DesktopWindow::onDesktopPreferences);
}

void DesktopWindow::onDesktopPreferences() {
    static_cast<Application* >(qApp)->desktopPrefrences();
}

void DesktopWindow::onFilerPreferences()
{
    static_cast<Application* >(qApp)->preferences(QString());
}

void DesktopWindow::onGoUp()
{
    FmPath* path = fm_path_new_for_path(XdgDir::readDesktopDir().toStdString().c_str());
    FmPath* parent = fm_path_get_parent(path);
    if (parent)
      onOpenFolder(fm_path_to_str(parent));
}

void DesktopWindow::onNewFolder()
{
    FmPath* path = fm_path_new_for_path(XdgDir::readDesktopDir().toStdString().c_str());
    createFileOrFolder(Fm::CreateNewFolder, path);
}

void DesktopWindow::onNewBlankFile()
{
    FmPath* path = fm_path_new_for_path(XdgDir::readDesktopDir().toStdString().c_str());
    createFileOrFolder(Fm::CreateNewTextFile, path);
}

void DesktopWindow::onOpenTerminal()
{
    FmPath* path = fm_path_new_for_path(XdgDir::readDesktopDir().toStdString().c_str());
    Application* app = static_cast<Application*>(qApp);
    app->openFolderInTerminal(path);
}

void DesktopWindow::onFindFiles()
{
    Application* app = static_cast<Application*>(qApp);
    FmPathList* selectedPaths = selectedFilePaths();
    QStringList paths;
    if(selectedPaths) {
      for(GList* l = fm_path_list_peek_head_link(selectedPaths); l; l = l->next) {
        // FIXME: is it ok to use display name here?
        // This might be broken on filesystems with non-UTF-8 filenames.
        Fm::Path path(FM_PATH(l->data));
        paths.append(path.displayName(false));
      }
      fm_path_list_unref(selectedPaths);
    }
    else {
      paths.append(XdgDir::readDesktopDir().toStdString().c_str());
    }
    app->findFiles(paths);
}

void DesktopWindow::onAbout()
{
    // the about dialog
    class AboutDialog : public QDialog {
    public:
      explicit AboutDialog(QWidget* parent = 0, Qt::WindowFlags f = 0) {
        ui.setupUi(this);
        ui.version->setText(tr("Version: %1").arg(PCMANFM_QT_VERSION));
      }
    private:
      Ui::AboutDialog ui;
    };
    AboutDialog dialog(this);
    dialog.exec();
}

void DesktopWindow::onEditBookmarks()
{
    Application* app = static_cast<Application*>(qApp);
    app->editBookmarks();
}

void DesktopWindow::onShowHidden(bool hidden)
{
    proxyModel_->setShowHidden(hidden);
}

void DesktopWindow::onSortColumn(int column)
{
    proxyModel_->sort(column, proxyModel_->sortOrder());
}

void DesktopWindow::onSortOrder(Qt::SortOrder order)
{
    proxyModel_->sort(proxyModel_->sortColumn(), order);
}

void DesktopWindow::onFolderFirst(bool first)
{
    proxyModel_->setFolderFirst(first);
}

void DesktopWindow::onCaseSensitive(Qt::CaseSensitivity sensitivity)
{
    proxyModel_->setSortCaseSensitivity(sensitivity);
}

void DesktopWindow::onReload()
{
  if(folder_) {
    fm_folder_reload(folder_);
  }
}

void DesktopWindow::updateMenu()
{
    desktopMainWindow_->setShowHidden(proxyModel_->showHidden());
    desktopMainWindow_->setSortColumn(proxyModel_->sortColumn());
    desktopMainWindow_->setSortOrder(proxyModel_->sortOrder());
    desktopMainWindow_->setFolderFirst(proxyModel_->folderFirst());
    desktopMainWindow_->setCaseSensitive(proxyModel_->sortCaseSensitivity());
}

void DesktopWindow::onRowsInserted(const QModelIndex& parent, int start, int end) {
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
    }
    queueRelayout();
}

void DesktopWindow::onLayoutChanged() {
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
        workArea = qApp->primaryScreen()->availableGeometry();
        workArea.adjust(12, 12, -12, -12); // add a 12 pixel margin to the work area
        // qDebug() << "workArea" << screen <<  workArea;
        // FIXME: we use an internal class declared in a private header here, which is pretty bad.
        QSize grid = listView_->gridSize();
        QPoint pos = workArea.topRight(); // probono: Desktop icons on the right-hand side
        pos.setX(pos.x() - grid.width() ); // probono: Desktop icons on the right-hand side
        for(; row < rowCount; ++row) {
            QModelIndex index = proxyModel_->index(row, 0);
            FmFileInfo* file = proxyModel_->fileInfoFromIndex(index);
            QByteArray name = fm_file_info_get_name(file);
            QHash<QByteArray, QPoint>::iterator it = customItemPos_.find(name);
            if(it != customItemPos_.end()) { // the item has a custom position
                QPoint customPos = *it;
                listView_->setPositionForIndex(customPos, index);
                qDebug() << "set custom pos:" << name << row << index << customPos;
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
            // probono:  Draw trash in bottom-right position
            if(name == "trash-can.desktop") {
                qDebug() << "probono: Trash fm_file_info_get_name: " << name;
                qDebug() << "probono: Draw trash in bottom-right position";
                QPoint trashPos = workArea.topRight();
                trashPos.setY(workArea.bottomRight().y() - grid.height() - listView_->spacing()); // probono
                trashPos.setX(trashPos.x() - grid.width());
                listView_->setPositionForIndex(trashPos, index);
            } else {
                listView_->setPositionForIndex(pos, index);
                if(used) { // go to next pos
                    --row;
                }
                // qDebug() << "set pos" << name << row << index << pos;
                // move to next cell in the column
                pos.setY(pos.y() + grid.height() + listView_->spacing());
                if(pos.y() + grid.height() * 1.5  > workArea.bottom()) { // probono: The 1.5 factor was added so that we have the last line exclusively for the trash. TODO: Find better solution?
                    // if the next position may exceed the bottom of work area, go to the top of next column
                    pos.setX(pos.x() - grid.width() - listView_->spacing()); // probono: Desktop icons on the right-hand side
                    pos.setY(workArea.top());

                    // check if the new column exceeds the right margin of work area
                    if(pos.x() + grid.width() > workArea.right()) {
                        if(desktop->isVirtualDesktop()) {
                            // in virtual desktop mode, go to next screen
                            ++screen;
                            break;
                        }
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

void DesktopWindow::queueRelayout(int delay) {
    // qDebug() << "queueRelayout";
    if(!relayoutTimer_) {
        relayoutTimer_ = new QTimer();
        relayoutTimer_->setSingleShot(true);
        connect(relayoutTimer_, &QTimer::timeout, this, &DesktopWindow::relayoutItems);
        relayoutTimer_->start(delay);
    }
}

// slots for file operations

void DesktopWindow::onCutActivated() {
    if(FmPathList* paths = selectedFilePaths()) {
        Fm::cutFilesToClipboard(paths);
        fm_path_list_unref(paths);
    }
}

void DesktopWindow::onCopyActivated() {
    if(FmPathList* paths = selectedFilePaths()) {
        Fm::copyFilesToClipboard(paths);
        fm_path_list_unref(paths);
    }
}

void DesktopWindow::onPasteActivated() {
    Fm::pasteFilesFromClipboard(path());
}

void DesktopWindow::onDuplicateActivated() {
    DesktopWindow::onCopyActivated();
    DesktopWindow::onPasteActivated();
}

void DesktopWindow::onEmptyTrashActivated() {
    Fm::Trash::emptyTrash();
}

void DesktopWindow::onDeleteActivated() {
    if(FmPathList* paths = selectedFilePaths()) {
        Settings& settings = static_cast<Application*>(qApp)->settings();
        Fm::FileOperation::trashFiles(paths, settings.confirmTrash());
        fm_path_list_unref(paths);
    }
}

void DesktopWindow::onDeleteWithoutTrashActivated() {
    if(FmPathList* paths = selectedFilePaths()) {
        Settings& settings = static_cast<Application*>(qApp)->settings();
        Fm::FileOperation::deleteFiles(paths, settings.confirmDelete(), this);
        fm_path_list_unref(paths);
    }
}

void DesktopWindow::onRenameActivated() {
    if(FmFileInfoList* files = selectedFiles()) {
        for(GList* l = fm_file_info_list_peek_head_link(files); l; l = l->next) {
            FmFileInfo* info = FM_FILE_INFO(l->data);
            Fm::renameFile(info, NULL);
            fm_file_info_list_unref(files);
        }
    }
}

// probono
void DesktopWindow::onOpenActivated()
{
    if(FmFileInfoList* files = selectedFiles()) {
        if(View::fileLauncher()) {
            View::fileLauncher()->launchFiles(NULL, files);
        }
        else { // use the default launcher
            Fm::FileLauncher launcher;
            launcher.launchFiles(NULL, files);
        }
    }
}

void DesktopWindow::onFilePropertiesActivated() {
    if(FmFileInfoList* files = selectedFiles()) {
        Fm::FilePropsDialog::showForFiles(files);
        fm_file_info_list_unref(files);
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
        case QEvent::FontChange:
            if(model_)
                queueRelayout();
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

void DesktopWindow::onOpenFolder(QString folder)
{
    qDebug() << "DesktopWindow::onOpenFolder: " << folder;
    FmPath* path = fm_path_new_for_str(folder.toLocal8Bit().data());
    onOpenDirRequested(path, 0);
    fm_path_unref(path);
}

void DesktopWindow::onOpenTrash()
{
    onOpenDirRequested(fm_path_get_trash(), 0);
}

void DesktopWindow::onOpenDesktop()
{
    onOpenDirRequested(fm_path_get_desktop(), 0);
}

void DesktopWindow::onOpenDocuments()
{
    FmPath* path;
    path = fm_path_new_for_str(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation).toLocal8Bit().data());
    onOpenDirRequested(path, 0);
    fm_path_unref(path);
}

void DesktopWindow::onOpenDownloads()
{
    FmPath* path;
    path = fm_path_new_for_str(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation).toLocal8Bit().data());
    onOpenDirRequested(path, 0);
    fm_path_unref(path);
}

void DesktopWindow::onOpenHome()
{
    onOpenDirRequested(fm_path_get_home(), 0);
}

void DesktopWindow::setScreenNum(int num) {
    if(screenNum_ != num) {
        screenNum_ = num;
        queueRelayout();
    }
}
