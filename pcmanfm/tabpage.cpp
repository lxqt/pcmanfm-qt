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


#include "tabpage.h"
#include "launcher.h"
#include <libfm-qt/filemenu.h>
#include <libfm-qt/mountoperation.h>
#include <libfm-qt/proxyfoldermodel.h>
#include <libfm-qt/cachedfoldermodel.h>
#include <libfm-qt/core/fileinfo.h>
#include <libfm-qt/utilities.h>
#include <QApplication>
#include <QCursor>
#include <QMessageBox>
#include <QScrollBar>
#include <QDir>
#include "settings.h"
#include "application.h"
#include <QTimer>
#include <QTextStream>
#include <QDebug>

using namespace Fm;

namespace PCManFM {

bool ProxyFilter::filterAcceptsRow(const Fm::ProxyFolderModel* model, const std::shared_ptr<const Fm::FileInfo>& info) const {
    if(!model || !info) {
        return true;
    }
    QString baseName = QString::fromStdString(info->name());
    if(!virtHiddenList_.isEmpty() && !model->showHidden() && virtHiddenList_.contains(baseName)) {
        return false;
    }
    if(!filterStr_.isEmpty() && !baseName.contains(filterStr_, Qt::CaseInsensitive)) {
        return false;
    }
    return true;
}

void ProxyFilter::setVirtHidden(const std::shared_ptr<Fm::Folder> &folder) {
    virtHiddenList_ = QStringList(); // reset the list
    if(!folder) {
        return;
    }
    auto path = folder->path();
    if(path) {
        auto pathStr = path.localPath();
        if(pathStr) {
            QString dotHidden = QString::fromUtf8(pathStr.get()) + QString("/.hidden");
            // FIXME: this does not work for non-local filesystems
            QFile file(dotHidden);
            if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                while(!in.atEnd()) {
                    virtHiddenList_.append(in.readLine());
                }
                file.close();
            }
        }
    }
}

TabPage::TabPage(QWidget* parent):
    QWidget(parent),
    folderView_{nullptr},
    folderModel_{nullptr},
    proxyModel_{nullptr},
    proxyFilter_{nullptr},
    verticalLayout{nullptr},
    overrideCursor_(false) {

    Settings& settings = static_cast<Application*>(qApp)->settings();

    // create proxy folder model to do item filtering
    proxyModel_ = new ProxyFolderModel();
    proxyModel_->setShowHidden(settings.showHidden());
    proxyModel_->setShowThumbnails(settings.showThumbnails());
    connect(proxyModel_, &ProxyFolderModel::sortFilterChanged, this, &TabPage::sortFilterChanged);

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    folderView_ = new View(settings.viewMode(), this);
    folderView_->setMargins(settings.folderViewCellMargins());
    // newView->setColumnWidth(Fm::FolderModel::ColumnName, 200);
    connect(folderView_, &View::openDirRequested, this, &TabPage::openDirRequested);
    connect(folderView_, &View::selChanged, this, &TabPage::onSelChanged);
    connect(folderView_, &View::clickedBack, this, &TabPage::backwardRequested);
    connect(folderView_, &View::clickedForward, this, &TabPage::forwardRequested);

    proxyFilter_ = new ProxyFilter();
    proxyModel_->addFilter(proxyFilter_);

    // FIXME: this is very dirty
    folderView_->setModel(proxyModel_);
    verticalLayout->addWidget(folderView_);
}

TabPage::~TabPage() {
    freeFolder();
    if(proxyFilter_) {
        delete proxyFilter_;
    }
    if(proxyModel_) {
        delete proxyModel_;
    }
    if(folderModel_) {
        folderModel_->unref();
    }

    if(overrideCursor_) {
        QApplication::restoreOverrideCursor(); // remove busy cursor
    }
}

void TabPage::freeFolder() {
    if(folder_) {
        if(folderSettings_.isCustomized()) {
            // save custom view settings for this folder
            static_cast<Application*>(qApp)->settings().saveFolderSettings(folder_->path(), folderSettings_);
        }
        disconnect(folder_.get(), nullptr, this, nullptr); // disconnect from all signals
        folder_ = nullptr;
    }
}

void TabPage::onFolderStartLoading() {
    if(!overrideCursor_) {
        // FIXME: sometimes FmFolder of libfm generates unpaired "start-loading" and
        // "finish-loading" signals of uncertain reasons. This should be a bug in libfm.
        // Until it's fixed in libfm, we need to workaround the problem here, not to
        // override the cursor twice.
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        overrideCursor_ = true;
    }
#if 0
#if FM_CHECK_VERSION(1, 0, 2) && 0 // disabled
    if(fm_folder_is_incremental(_folder)) {
        /* create a model for the folder and set it to the view
           it is delayed for non-incremental folders since adding rows into
           model is much faster without handlers connected to its signals */
        FmFolderModel* model = fm_folder_model_new(folder, FALSE);
        fm_folder_view_set_model(fv, model);
        fm_folder_model_set_sort(model, app_config->sort_by,
                                 (app_config->sort_type == GTK_SORT_ASCENDING) ?
                                 FM_SORT_ASCENDING : FM_SORT_DESCENDING);
        g_object_unref(model);
    }
    else
#endif
        fm_folder_view_set_model(fv, nullptr);
#endif
}

void TabPage::restoreScrollPos() {
    // scroll to recorded position
    folderView_->childView()->verticalScrollBar()->setValue(browseHistory().currentScrollPos());

    // if the current folder is the parent folder of the last browsed folder,
    // select the folder item in current view.
    if(lastFolderPath_ && lastFolderPath_.parent() == path()) {
        QModelIndex index = folderView_->indexFromFolderPath(lastFolderPath_);
        if(index.isValid()) {
            folderView_->childView()->scrollTo(index, QAbstractItemView::EnsureVisible);
            folderView_->childView()->setCurrentIndex(index);
        }
    }
}

void TabPage::onFolderFinishLoading() {
    auto fi = folder_->info();
    if(fi) { // if loading of the folder fails, it's possible that we don't have FmFileInfo.
        setWindowTitle(fi->displayName());
        Q_EMIT titleChanged(fi->displayName());
    }

    folder_->queryFilesystemInfo(); // FIXME: is this needed?
#if 0
    FmFolderView* fv = folder_view;
    const FmNavHistoryItem* item;
    GtkScrolledWindow* scroll = GTK_SCROLLED_WINDOW(fv);

    /* Note: most of the time, we delay the creation of the
     * folder model and do it after the whole folder is loaded.
     * That is because adding rows into model is much faster when no handlers
     * are connected to its signals. So we detach the model from folder view
     * and create the model again when it's fully loaded.
     * This optimization, however, is not used for FmFolder objects
     * with incremental loading (search://) */
    if(fm_folder_view_get_model(fv) == nullptr) {
        /* create a model for the folder and set it to the view */
        FmFolderModel* model = fm_folder_model_new(folder, app_config->show_hidden);
        fm_folder_view_set_model(fv, model);
#if FM_CHECK_VERSION(1, 0, 2)
        /* since 1.0.2 sorting should be applied on model instead of view */
        fm_folder_model_set_sort(model, app_config->sort_by,
                                 (app_config->sort_type == GTK_SORT_ASCENDING) ?
                                 FM_SORT_ASCENDING : FM_SORT_DESCENDING);
#endif
        g_object_unref(model);
    }

#endif

    // update status text
    QString& text = statusText_[StatusTextNormal];
    text = formatStatusText();
    Q_EMIT statusChanged(StatusTextNormal, text);

    if(overrideCursor_) {
        QApplication::restoreOverrideCursor(); // remove busy cursor
        overrideCursor_ = false;
    }

    // After finishing loading the folder, the model is updated, but Qt delays the UI update
    // for performance reasons. Therefore at this point the UI is not up to date.
    // Of course, the scrollbar ranges are not updated yet. We solve this by installing an Qt timeout handler.
    QTimer::singleShot(10, this, SLOT(restoreScrollPos()));
}

void TabPage::onFolderError(const Fm::GErrorPtr& err, Fm::Job::ErrorSeverity severity, Fm::Job::ErrorAction& response) {
    if(err.domain() == G_IO_ERROR) {
        if(err.code() == G_IO_ERROR_NOT_MOUNTED && severity < Fm::Job::ErrorSeverity::CRITICAL) {
            auto& path = folder_->path();
            MountOperation* op = new MountOperation(this);
            op->mount(path);
            if(op->wait()) { // blocking event loop, wait for mount operation to finish.
                // This will reload the folder, which generates a new "start-loading"
                // signal, so we get more "start-loading" signals than "finish-loading"
                // signals. FIXME: This is a bug of libfm.
                // Because the two signals are not correctly paired, we need to
                // remove busy cursor here since "finish-loading" is not emitted.
                QApplication::restoreOverrideCursor(); // remove busy cursor
                overrideCursor_ = false;
                response = Fm::Job::ErrorAction::RETRY;
                return;
            }
        }
    }
    if(severity >= Fm::Job::ErrorSeverity::MODERATE) {
        /* Only show more severe errors to the users and
          * ignore milder errors. Otherwise too many error
          * message boxes can be annoying.
          * This fixes bug #3411298- Show "Permission denied" when switching to super user mode.
          * https://sourceforge.net/tracker/?func=detail&aid=3411298&group_id=156956&atid=801864
          * */

        // FIXME: consider replacing this modal dialog with an info bar to improve usability
        QMessageBox::critical(this, tr("Error"), err.message());
    }
    response = Fm::Job::ErrorAction::CONTINUE;
}

void TabPage::onFolderFsInfo() {
    guint64 free, total;
    QString& msg = statusText_[StatusTextFSInfo];
    if(folder_->getFilesystemInfo(&total, &free)) {
        char total_str[64];
        char free_str[64];
        fm_file_size_to_str(free_str, sizeof(free_str), free, fm_config->si_unit);
        fm_file_size_to_str(total_str, sizeof(total_str), total, fm_config->si_unit);
        msg = tr("Free space: %1 (Total: %2)")
              .arg(QString::fromUtf8(free_str))
              .arg(QString::fromUtf8(total_str));
    }
    else {
        msg.clear();
    }
    Q_EMIT statusChanged(StatusTextFSInfo, msg);
}

QString TabPage::formatStatusText() {
    if(proxyModel_ && folder_) {
        // FIXME: this is very inefficient
        auto files = folder_->files();
        int total_files = files.size();
        int shown_files = proxyModel_->rowCount();
        int hidden_files = total_files - shown_files;
        QString text = tr("%n item(s)", "", shown_files);
        if(hidden_files > 0) {
            text += tr(" (%n hidden)", "", hidden_files);
        }
        return text;
    }
    return QString();
}

void TabPage::onFolderRemoved() {
    // the folder we're showing is removed, destroy the widget
    qDebug("folder removed");
    Settings& settings = static_cast<Application*>(qApp)->settings();
    // NOTE: call deleteLater() directly from this GObject signal handler
    // does not work but I don't know why.
    // Maybe it's the problem of glib mainloop integration?
    // Call it when idle works, though.
    if(settings.closeOnUnmount()) {
        QTimer::singleShot(0, this, SLOT(deleteLater()));
    }
    else {
        chdir(Fm::FilePath::homeDir());
    }
}

void TabPage::onFolderUnmount() {
    // the folder we're showing is unmounted, destroy the widget
    qDebug("folder unmount");
    // NOTE: call deleteLater() directly from this GObject signal handler
    // does not work but I don't know why.
    // Maybe it's the problem of glib mainloop integration?
    // Call it when idle works, though.
    Settings& settings = static_cast<Application*>(qApp)->settings();
    // NOTE: call deleteLater() directly from this GObject signal handler
    // does not work but I don't know why.
    // Maybe it's the problem of glib mainloop integration?
    // Call it when idle works, though.
    if(settings.closeOnUnmount()) {
        QTimer::singleShot(0, this, SLOT(deleteLater()));
    }
    else {
        chdir(Fm::FilePath::homeDir());
    }
}

void TabPage::onFolderContentChanged() {
    /* update status text */
    statusText_[StatusTextNormal] = formatStatusText();
    Q_EMIT statusChanged(StatusTextNormal, statusText_[StatusTextNormal]);
}

QString TabPage::pathName() {
    // auto disp_path = path().displayName();
    // FIXME: displayName() returns invalid path sometimes.
    auto disp_path = path().toString();
    return QString::fromUtf8(disp_path.get());
}

void TabPage::chdir(Fm::FilePath newPath, bool addHistory) {
    // qDebug() << "TABPAGE CHDIR:" << newPath.toString().get();
    if(folder_) {
        // we're already in the specified dir
        if(newPath == folder_->path()) {
            return;
        }

        // remember the previous folder path that we have browsed.
        lastFolderPath_ = folder_->path();

        if(addHistory) {
            // store current scroll pos in the browse history
            BrowseHistoryItem& item = history_.currentItem();
            QAbstractItemView* childView = folderView_->childView();
            item.setScrollPos(childView->verticalScrollBar()->value());
        }

        // free the previous model
        if(folderModel_) {
            proxyModel_->setSourceModel(nullptr);
            folderModel_->unref(); // unref the cached model
            folderModel_ = nullptr;
        }

        freeFolder();
    }

    Q_EMIT titleChanged(newPath.baseName().get());  // FIXME: display name

    folder_ = Fm::Folder::fromPath(newPath);
    proxyFilter_->setVirtHidden(folder_);
    if(addHistory) {
        // add current path to browse history
        history_.add(path());
    }
    connect(folder_.get(), &Fm::Folder::startLoading, this, &TabPage::onFolderStartLoading);
    connect(folder_.get(), &Fm::Folder::finishLoading, this, &TabPage::onFolderFinishLoading);

    // FIXME: Fm::Folder::error() is a bad design and might be removed in the future.
    connect(folder_.get(), &Fm::Folder::error, this, &TabPage::onFolderError);
    connect(folder_.get(), &Fm::Folder::fileSystemChanged, this, &TabPage::onFolderFsInfo);
    /* destroy the page when the folder is unmounted or deleted. */
    connect(folder_.get(), &Fm::Folder::removed, this, &TabPage::onFolderRemoved);
    connect(folder_.get(), &Fm::Folder::unmount, this, &TabPage::onFolderUnmount);
    connect(folder_.get(), &Fm::Folder::contentChanged, this, &TabPage::onFolderContentChanged);

    folderModel_ = CachedFolderModel::modelFromFolder(folder_);

    // set sorting, considering customized folders
    Settings& settings = static_cast<Application*>(qApp)->settings();
    folderSettings_ = settings.loadFolderSettings(path());
    proxyModel_->sort(folderSettings_.sortColumn(), folderSettings_.sortOrder());
    proxyModel_->setFolderFirst(folderSettings_.sortFolderFirst());
    proxyModel_->setShowHidden(folderSettings_.showHidden());
    proxyModel_->setSortCaseSensitivity(folderSettings_.sortCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    proxyModel_->setSourceModel(folderModel_);

    folderView_->setViewMode(folderSettings_.viewMode());

    if(folder_->isLoaded()) {
        onFolderStartLoading();
        onFolderFinishLoading();
        onFolderFsInfo();
    }
    else {
        onFolderStartLoading();
    }
}

void TabPage::selectAll() {
    folderView_->selectAll();
}

void TabPage::invertSelection() {
    folderView_->invertSelection();
}

void TabPage::reload() {
    if(folder_) {
        proxyFilter_->setVirtHidden(folder_); // reread ".hidden"
        folder_->reload();
    }
}

// when the current selection in the folder view is changed
void TabPage::onSelChanged() {
    QString msg;
    if(folderView_->hasSelection()) {
        auto files = folderView_->selectedFiles();
        int numSel = files.size();
        /* FIXME: display total size of all selected files. */
        if(numSel == 1) { /* only one file is selected */
            auto& fi = files.front();
            if(!fi->isDir()) {
                msg = QString("\"%1\" (%2) %3")
                      .arg(fi->displayName())
                      .arg(Fm::formatFileSize(fi->size(), fm_config->si_unit)) // FIXME: deprecate fm_config
                      .arg(fi->mimeType()->desc());
            }
            else {
                msg = QString("\"%1\" %2")
                      .arg(fi->displayName())
                      .arg(fi->mimeType()->desc());
            }
            /* FIXME: should we support statusbar plugins as in the gtk+ version? */
        }
        else {
            goffset sum;
            GList* l;
            msg = tr("%n item(s) selected", nullptr, numSel);
            /* don't count if too many files are selected, that isn't lightweight */
            if(numSel < 1000) {
                sum = 0;
                for(auto& fi: files) {
                    if(fi->isDir()) {
                        /* if we got a directory then we cannot tell it's size
                        unless we do deep count but we cannot afford it */
                        sum = -1;
                        break;
                    }
                    sum += fi->size();
                }
                if(sum >= 0) {
                    msg += QString(" (%1)").arg(Fm::formatFileSize(sum, fm_config->si_unit)); // FIXME: deprecate fm_config
                }
                /* FIXME: should we support statusbar plugins as in the gtk+ version? */
            }
            /* FIXME: can we show some more info on selection?
                that isn't lightweight if a lot of files are selected */
        }
    }
    statusText_[StatusTextSelectedFiles] = msg;
    Q_EMIT statusChanged(StatusTextSelectedFiles, msg);
}


void TabPage::backward() {
    // remember current scroll position
    BrowseHistoryItem& item = history_.currentItem();
    QAbstractItemView* childView = folderView_->childView();
    item.setScrollPos(childView->verticalScrollBar()->value());

    history_.backward();
    chdir(history_.currentPath(), false);
}

void TabPage::forward() {
    // remember current scroll position
    BrowseHistoryItem& item = history_.currentItem();
    QAbstractItemView* childView = folderView_->childView();
    item.setScrollPos(childView->verticalScrollBar()->value());

    history_.forward();
    chdir(history_.currentPath(), false);
}

void TabPage::jumpToHistory(int index) {
    if(index >= 0 && index < history_.size()) {
        // remember current scroll position
        BrowseHistoryItem& item = history_.currentItem();
        QAbstractItemView* childView = folderView_->childView();
        item.setScrollPos(childView->verticalScrollBar()->value());

        history_.setCurrentIndex(index);
        chdir(history_.currentPath(), false);
    }
}

bool TabPage::canUp() {
    auto _path = path();
    return (_path && _path.hasParent());
}

void TabPage::up() {
    auto _path = path();
    if(_path) {
        auto parent = _path.parent();
        if(parent) {
            chdir(parent, true);
        }
    }
}

void TabPage::updateFromSettings(Settings& settings) {
    folderView_->updateFromSettings(settings);
}

void TabPage::setViewMode(Fm::FolderView::ViewMode mode) {
    if(folderSettings_.viewMode() != mode) {
        folderSettings_.setViewMode(mode);
        if(folderSettings_.isCustomized()) {
            static_cast<Application*>(qApp)->settings().saveFolderSettings(path(), folderSettings_);
        }
    }
    folderView_->setViewMode(mode);
}

void TabPage::sort(int col, Qt::SortOrder order) {
    if(folderSettings_.sortColumn() != col || folderSettings_.sortOrder() != order) {
        folderSettings_.setSortColumn(Fm::FolderModel::ColumnId(col));
        folderSettings_.setSortOrder(order);
        if(folderSettings_.isCustomized()) {
            static_cast<Application*>(qApp)->settings().saveFolderSettings(path(), folderSettings_);
        }
    }
    if(proxyModel_) {
        proxyModel_->sort(col, order);
    }
}

void TabPage::setSortFolderFirst(bool value) {
    if(folderSettings_.sortFolderFirst() != value) {
        folderSettings_.setSortFolderFirst(value);
        if(folderSettings_.isCustomized()) {
            static_cast<Application*>(qApp)->settings().saveFolderSettings(path(), folderSettings_);
        }
    }
    proxyModel_->setFolderFirst(value);
}

void TabPage::setSortCaseSensitive(bool value) {
    if(folderSettings_.sortCaseSensitive() != value) {
        folderSettings_.setSortCaseSensitive(value);
        if(folderSettings_.isCustomized()) {
            static_cast<Application*>(qApp)->settings().saveFolderSettings(path(), folderSettings_);
        }
    }
    proxyModel_->setSortCaseSensitivity(value ? Qt::CaseSensitive : Qt::CaseInsensitive);
}


void TabPage::setShowHidden(bool showHidden) {
    if(folderSettings_.showHidden() != showHidden) {
        folderSettings_.setShowHidden(showHidden);
        if(folderSettings_.isCustomized()) {
            static_cast<Application*>(qApp)->settings().saveFolderSettings(path(), folderSettings_);
        }
    }
    if(!proxyModel_ || showHidden == proxyModel_->showHidden()) {
        return;
    }
    proxyModel_->setShowHidden(showHidden);
    statusText_[StatusTextNormal] = formatStatusText();
    Q_EMIT statusChanged(StatusTextNormal, statusText_[StatusTextNormal]);
}

void TabPage::applyFilter() {
    if(!proxyModel_) {
        return;
    }
    proxyModel_->updateFilters();
    statusText_[StatusTextNormal] = formatStatusText();
    Q_EMIT statusChanged(StatusTextNormal, statusText_[StatusTextNormal]);
}

void TabPage::setCustomizedView(bool value) {
    if(folderSettings_.isCustomized() == value) {
        return;
    }

    Settings& settings = static_cast<Application*>(qApp)->settings();
    folderSettings_.setCustomized(value);
    if(value) { // save customized folder view settings
        settings.saveFolderSettings(path(), folderSettings_);
    }
    else { // use default folder view settings
        settings.clearFolderSettings(path());
        setShowHidden(settings.showHidden());
        setSortCaseSensitive(settings.sortCaseSensitive());
        setSortFolderFirst(settings.sortFolderFirst());
        sort(settings.sortColumn(), settings.sortOrder());
    }
}

} // namespace PCManFM
