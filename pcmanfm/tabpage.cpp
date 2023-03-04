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
#include <QToolButton>
#include <QLabel>
#include <QToolTip>
#include <QDir>
#include <QStandardPaths>
#include "settings.h"
#include "application.h"
#include "desktopentrydialog.h"
#include <QTimer>
#include <QDebug>

using namespace Fm;

namespace PCManFM {

bool ProxyFilter::filterAcceptsRow(const Fm::ProxyFolderModel* model, const std::shared_ptr<const Fm::FileInfo>& info) const {
    if(!model || !info) {
        return true;
    }
    QString baseName = fullName_ && !info->name().empty() ? QString::fromStdString(info->name())
                                                          : info->displayName();
    if(!filterStr_.isEmpty() && !baseName.contains(filterStr_, Qt::CaseInsensitive)) {
        return false;
    }
    return true;
}

//==================================================

FilterEdit::FilterEdit(QWidget* parent) : QLineEdit(parent) {
    setClearButtonEnabled(true);
    if(QToolButton *clearButton = findChild<QToolButton*>()) {
        clearButton->setToolTip(tr("Clear text (Ctrl+K or Esc)"));
    }
}

void FilterEdit::keyPressEvent(QKeyEvent* event) {
    // since two views can be shown in the split mode, Ctrl+K can't be
    // used as a QShortcut but can come here for clearing the text
    if(event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_K) {
        clear();
    }
    QLineEdit::keyPressEvent(event);
}

void FilterEdit::keyPressed(QKeyEvent* event) {
    // NOTE: Movement and delete keys should be left to the view.
    // Copy/paste shortcuts are taken by the view but they aren't needed here
    // (Shift+Insert works for pasting but, since most users may not be familiar
    // with it, an action is added to the main window for focusing an empty bar).
    if(!hasFocus()
       && event->key() != Qt::Key_Left && event->key() != Qt::Key_Right
       && event->key() != Qt::Key_Home && event->key() != Qt::Key_End
       && event->key() != Qt::Key_Delete) {
        keyPressEvent(event);
    }
}

FilterBar::FilterBar(QWidget* parent) : QWidget(parent) {
    QHBoxLayout* HLayout = new QHBoxLayout(this);
    HLayout->setSpacing(5);
    filterEdit_ = new FilterEdit();
    QLabel *label = new QLabel(tr("Filter:"));
    HLayout->addWidget(label);
    HLayout->addWidget(filterEdit_);
    connect(filterEdit_, &QLineEdit::textChanged, this, &FilterBar::textChanged);
    connect(filterEdit_, &FilterEdit::lostFocus, this, &FilterBar::lostFocus);
}

//==================================================

TabPage::TabPage(QWidget* parent):
    QWidget(parent),
    folderView_{nullptr},
    folderModel_{nullptr},
    proxyModel_{nullptr},
    proxyFilter_{nullptr},
    verticalLayout{nullptr},
    overrideCursor_(false),
    selectionTimer_(nullptr),
    filterBar_(nullptr) {

    Settings& settings = static_cast<Application*>(qApp)->settings();

    // create proxy folder model to do item filtering
    proxyModel_ = new ProxyFolderModel();
    proxyModel_->setShowHidden(settings.showHidden());
    proxyModel_->setBackupAsHidden(settings.backupAsHidden());
    proxyModel_->setShowThumbnails(settings.showThumbnails());
    connect(proxyModel_, &ProxyFolderModel::sortFilterChanged, this, [this] {
        QToolTip::showText(QPoint(), QString()); // remove the tooltip, if any
        saveFolderSorting();
        Q_EMIT sortFilterChanged();
    });

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    folderView_ = new View(settings.viewMode(), this);
    folderView_->setMargins(settings.folderViewCellMargins());
    folderView_->setShadowHidden(settings.shadowHidden());
    // newView->setColumnWidth(Fm::FolderModel::ColumnName, 200);
    connect(folderView_, &View::selChanged, this, &TabPage::onSelChanged);
    connect(folderView_, &View::clickedBack, this, &TabPage::backwardRequested);
    connect(folderView_, &View::clickedForward, this, &TabPage::forwardRequested);

    // customization of columns of detailed list view
    folderView_->setCustomColumnWidths(settings.getCustomColumnWidths());
    folderView_->setHiddenColumns(settings.getHiddenColumns());
    connect(folderView_, &View::columnResizedByUser, this, [this, &settings]() {
        settings.setCustomColumnWidths(folderView_->getCustomColumnWidths());
    });
    connect(folderView_, &View::columnHiddenByUser, this, [this, &settings]() {
        settings.setHiddenColumns(folderView_->getHiddenColumns());
    });

    proxyFilter_ = new ProxyFilter();
    proxyModel_->addFilter(proxyFilter_);

    // FIXME: this is very dirty
    folderView_->setModel(proxyModel_);
    verticalLayout->addWidget(folderView_);

    folderView_->childView()->installEventFilter(this);
    if(settings.noItemTooltip()) {
        folderView_->childView()->viewport()->installEventFilter(this);
    }

    // filter-bar and its settings
    filterBar_ = new FilterBar();
    verticalLayout->addWidget(filterBar_);
    if(!settings.showFilter()){
        transientFilterBar(true);
    }
    connect(filterBar_, &FilterBar::textChanged, this, &TabPage::onFilterStringChanged);
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
        disconnect(folderModel_, &Fm::FolderModel::fileSizeChanged, this, &TabPage::onFileSizeChanged);
        disconnect(folderModel_, &Fm::FolderModel::filesAdded, this, &TabPage::onFilesAdded);
        folderModel_->unref();
    }

    if(overrideCursor_) {
        QApplication::restoreOverrideCursor(); // remove busy cursor
    }
}

void TabPage::transientFilterBar(bool transient) {
    if(filterBar_) {
        filterBar_->clear();
        if(transient) {
            filterBar_->hide();
            connect(filterBar_, &FilterBar::lostFocus, this, &TabPage::onLosingFilterBarFocus);
        }
        else {
            filterBar_->show();
            disconnect(filterBar_, &FilterBar::lostFocus, this, &TabPage::onLosingFilterBarFocus);
        }
    }
}

void TabPage::onLosingFilterBarFocus() {
    // hide the empty transient filter-bar when it loses focus
    if(getFilterStr().isEmpty()) {
        filterBar_->hide();
    }
}

void TabPage::showFilterBar() {
    if(filterBar_) {
        filterBar_->show();
        if(isVisibleTo(this)) { // the page itself may be in an inactive tab
            filterBar_->focusBar();
        }
    }
}

bool TabPage::eventFilter(QObject* watched, QEvent* event) {
    if(watched == folderView_->childView() && event->type() == QEvent::KeyPress) {
        QToolTip::showText(QPoint(), QString()); // remove the tooltip, if any
        // when a text is typed inside the view, type it inside the transient filter-bar
        if(filterBar_ && !static_cast<Application*>(qApp)->settings().showFilter()) {
            if(QKeyEvent* ke = static_cast<QKeyEvent*>(event)) {
                filterBar_->keyPressed(ke);
            }
        }
    }
    else if (watched == folderView_->childView()->viewport() && event->type() == QEvent::ToolTip) {
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void TabPage::backspacePressed() {
    if(filterBar_ && filterBar_->isVisible()) {
        QKeyEvent bs = QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
        filterBar_->keyPressed(&bs);
    }
}

void TabPage::onFilterStringChanged(QString str) {
    if(filterBar_ && str != getFilterStr()) {
        QToolTip::showText(QPoint(), QString()); // remove the tooltip, if any

        bool transientFilterBar = !static_cast<Application*>(qApp)->settings().showFilter();

        // with a transient filter-bar, let the current index be selected by Qt
        // if the first pressed key is a space
        if(transientFilterBar && !filterBar_->isVisibleTo(this)
           && folderView()->childView()->hasFocus()
           && str == QString(QChar(QChar::Space))) {
            QModelIndex index = folderView_->selectionModel()->currentIndex();
            if (index.isValid() && !folderView_->selectionModel()->isSelected(index)) {
                filterBar_->clear();
                folderView_->childView()->scrollTo(index, QAbstractItemView::EnsureVisible);
                return;
            }
        }

        setFilterStr(str);

        // Because the filter string may be typed inside the view, we should wait for Qt
        // to select an item before deciding about the selection in applyFilter().
        // Therefore, we use a single-shot timer to apply the filter.
        QTimer::singleShot(0, folderView_, [this] {
            applyFilter();
        });

        // show/hide the transient filter-bar appropriately
        if(transientFilterBar) {
            if(filterBar_->isVisibleTo(this)) { // the page itself may be in an inactive tab
                if(str.isEmpty()) {
                    // focus the view BEFORE hiding the filter-bar to avoid redundant "FocusIn" events;
                    // otherwise, another widget inside the main window might gain focus immediately
                    // after the filter-bar is hidden and only after that, the view will be focused.
                    folderView()->childView()->setFocus();
                    filterBar_->hide();
                }
            }
            else if(!str.isEmpty()) {
                filterBar_->show();
            }
        }
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
        filesToTrust_.clear();
    }
}

void TabPage::onFolderStartLoading() {
    if(folderModel_){
        disconnect(folderModel_, &Fm::FolderModel::filesAdded, this, &TabPage::onFilesAdded);
    }
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

void TabPage::onUiUpdated() {
    bool scrolled = false;
    // if there are files to select, select them
    if(!filesToSelect_.empty()) {
        Fm::FileInfoList infos;
        for(const auto& file : filesToSelect_) {
            if(auto info = proxyModel_->fileInfoFromPath(file)) {
                infos.push_back(info);
            }
        }
        filesToSelect_.clear();
        if(folderView_->selectFiles(infos)) {
            scrolled = true; // scrolling is done by FolderView::selectFiles()
            QModelIndexList indexes = folderView_->selectionModel()->selectedIndexes();
            if(!indexes.isEmpty()) {
                folderView_->selectionModel()->setCurrentIndex(indexes.first(), QItemSelectionModel::NoUpdate);
            }
        }
    }
    // if the current folder is the parent folder of the last browsed folder,
    // select the folder item in current view.
    if(!scrolled && lastFolderPath_ && lastFolderPath_.parent() == path()) {
        QModelIndex index = folderView_->indexFromFolderPath(lastFolderPath_);
        if(index.isValid()) {
            folderView_->childView()->scrollTo(index, QAbstractItemView::EnsureVisible);
            folderView_->childView()->setCurrentIndex(index);
            scrolled = true;
        }
    }
    if(!scrolled) {
        // set the first item as current
        QModelIndex firstIndx = proxyModel_->index(0, 0);
        if (firstIndx.isValid()) {
            folderView_->selectionModel()->setCurrentIndex(firstIndx, QItemSelectionModel::NoUpdate);
        }
        // scroll to recorded position
        folderView_->childView()->verticalScrollBar()->setValue(browseHistory().currentScrollPos());
    }

    if(folderModel_) {
        // update selection statusbar info when needed
        connect(folderModel_, &Fm::FolderModel::fileSizeChanged, this, &TabPage::onFileSizeChanged);
        // get ready to select files that may be added later
        connect(folderModel_, &Fm::FolderModel::filesAdded, this, &TabPage::onFilesAdded);
    }

    // in the single-click mode, set the cursor shape of the view to a pointing hand
    // only if there is an item under it
    if(folderView_->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick)) {
        QTimer::singleShot(0, folderView_, [this] {
            QPoint pos = folderView_->childView()->mapFromGlobal(QCursor::pos());
            QModelIndex index = folderView_->childView()->indexAt(pos);
            if(index.isValid()) {
                folderView_->setCursor(Qt::PointingHandCursor);
            }
            else {
                folderView_->setCursor(Qt::ArrowCursor);
            }
        });
    }
}

void TabPage::onFileSizeChanged(const QModelIndex& index) {
    if(folderView_->hasSelection()) {
        QModelIndexList indexes = folderView_->selectionModel()->selectedIndexes();
        if(indexes.contains(proxyModel_->mapFromSource(index))) {
            onSelChanged();
        }
    }
}

// slot
void TabPage::onFilesAdded(Fm::FileInfoList files) {
    if(static_cast<Application*>(qApp)->settings().selectNewFiles()) {
        if(!selectionTimer_) {
            selectionTimer_ = new QTimer (this);
            selectionTimer_->setSingleShot(true);
            if(folderView_->selectFiles(files, false)) {
                selectionTimer_->start(200);
            }
        }
        else if(folderView_->selectFiles(files, selectionTimer_->isActive())) {
            selectionTimer_->start(200);
        }
    }
    else if (!folderView_->selectionModel()->currentIndex().isValid()) {
        // set the first item as current if there is no current item
        QModelIndex firstIndx = proxyModel_->index(0, 0);
        if (firstIndx.isValid()) {
            folderView_->selectionModel()->setCurrentIndex(firstIndx, QItemSelectionModel::NoUpdate);
        }
    }

    // trust the files that are added by createShortcut()
    if(!filesToTrust_.isEmpty()) {
        for(const auto& file : files) {
            const QString fileName = QString::fromStdString(file->name());
            if(filesToTrust_.contains(fileName)) {
                file->setTrustable(true);
                filesToTrust_.removeAll(fileName);
                if(filesToTrust_.isEmpty()) {
                    break;
                }
            }
        }
    }
}

void TabPage::localizeTitle(const Fm::FilePath& path) {
    // force localization on some locations (FIXME: localize them in libfm-qt?)
    if(!path.isNative()) {
        if(path.hasUriScheme("search")) {
            title_ = tr("Search Results");
        }
        else if(strcmp(path.toString().get(), "menu://applications/") == 0) {
            title_ = tr("Applications");
        }
        else if(!path.hasParent()) {
            if(path.hasUriScheme("computer")) {
                title_ = tr("Computer");
            }
            else if(path.hasUriScheme("network")) {
                title_ = tr("Network");
            }
            else if(path.hasUriScheme("trash")) {
                title_ = tr("Trash");
            }
        }
    }
    else if(QString::fromUtf8(path.toString().get()) ==
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)) {
        title_ = tr("Desktop");
    }
}

void TabPage::onFolderFinishLoading() {
    auto fi = folder_->info();
    if(fi) { // if loading of the folder fails, it's possible that we don't have FmFileInfo.
        title_ = fi->displayName();
        localizeTitle(folder_->path());
        Q_EMIT titleChanged();
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
    // For example, the scrollbar ranges are not updated yet. We solve this by installing an Qt timeout handler.
    QTimer::singleShot(10, this, &TabPage::onUiUpdated);
}

void TabPage::onFolderError(const Fm::GErrorPtr& err, Fm::Job::ErrorSeverity severity, Fm::Job::ErrorAction& response) {
    if(err.domain() == G_IO_ERROR) {
        if(err.code() == G_IO_ERROR_NOT_MOUNTED && severity < Fm::Job::ErrorSeverity::CRITICAL) {
            auto& path = folder_->path();
            // WARNING: GVFS admin backend has a bug that tries to mount an admin path with
            // a double slash, like "admin://X", even when Admin is already mounted. The mount
            // is always completed successfully, so that it can cause an infinite loop here.
            // Since "admin" is already handled by canOpenAdmin(), it can be safely excluded
            // here, as a workaround.
            if(!path.hasUriScheme("admin")) {
                MountOperation* op = new MountOperation(true);
                op->mountEnclosingVolume(path);
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
        msg = tr("Free space: %1 (Total: %2)")
              .arg(formatFileSize(free, fm_config->si_unit),
                   formatFileSize(total, fm_config->si_unit));
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
        auto fi = folder_->info();
        if (fi && fi->isSymlink()) {
            text += QStringLiteral(" (%1)")
                    .arg(tr("Link to") + QChar(QChar::Space) + QString::fromStdString(fi->target()));
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
        QTimer::singleShot(0, this, &TabPage::deleteLater);
    }
    else {
        chdir(Fm::FilePath::homeDir());
    }
}

void TabPage::onFolderUnmount() {
    // the folder we're showing is unmounted, destroy the widget
    qDebug("folder unmount");
    // NOTE: We cannot delete the page or change its directory here
    // because unmounting might be done from places view, in which case,
    // the mount operation is a child of the places view and should be
    // finished before doing anything else.
    if(static_cast<Application*>(qApp)->settings().closeOnUnmount()) {
        freeFolder();
    }
    else if (folder_) {
        // the folder shouldn't be freed here because the dir will be changed by
        // the slot of MainWindow but it should be disconnected from all signals
        disconnect(folder_.get(), nullptr, this, nullptr);
    }
    Q_EMIT folderUnmounted();
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
    if(filterBar_){
        filterBar_->clear();
    }
    if(folder_) {
        // we're already in the specified dir
        if(newPath == folder_->path()) {
            return;
        }

        if (newPath.hasUriScheme("admin") && !canOpenAdmin()) {
            return; // see canOpenAdmin() for a thorough explanation
        }

        // reset the status selected text
        statusText_[StatusTextSelectedFiles] = QString();

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
            disconnect(folderModel_, &Fm::FolderModel::fileSizeChanged, this, &TabPage::onFileSizeChanged);
            disconnect(folderModel_, &Fm::FolderModel::filesAdded, this, &TabPage::onFilesAdded);
            proxyModel_->setSourceModel(nullptr);
            folderModel_->unref(); // unref the cached model
            folderModel_ = nullptr;
        }

        freeFolder();
    }

    // remove the tooltip, if any
    QToolTip::showText(QPoint(), QString());
    // set title as with path button (will change if the new folder is loaded)
    title_ = QString::fromUtf8(newPath.baseName().get());
    localizeTitle(newPath);
    Q_EMIT titleChanged();

    folder_ = Fm::Folder::fromPath(newPath);
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

    Settings& settings = static_cast<Application*>(qApp)->settings();
    folderModel_ = CachedFolderModel::modelFromFolder(folder_);
    // always show display names in special places because real names may be unusual
    // (e.g., real names of trashed files may contain trash path with backslash)
    if(!settings.showFullNames()
       || newPath.hasUriScheme("menu") || newPath.hasUriScheme("trash")
       || newPath.hasUriScheme("network") || newPath.hasUriScheme("computer")) {
        folderModel_->setShowFullName(false);
        proxyFilter_->filterFullName(false);
    }
    else {
        folderModel_->setShowFullName(true);
        proxyFilter_->filterFullName(true);
    }

    // folderSettings_ will be set by saveFolderSorting() when the sort filter is changed below
    // (and also by setViewMode()); here, we only need to know whether it should be saved
    FolderSettings folderSettings = settings.loadFolderSettings(path());
    folderSettings_.setCustomized(folderSettings.isCustomized());
    folderSettings_.setRecursive(folderSettings.recursive());
    folderSettings_.seInheritedPath(folderSettings.inheritedPath());

    // set sorting
    proxyModel_->sort(folderSettings.sortColumn(), folderSettings.sortOrder());
    proxyModel_->setFolderFirst(folderSettings.sortFolderFirst());
    proxyModel_->setHiddenLast(folderSettings.sortHiddenLast());
    proxyModel_->setShowHidden(folderSettings.showHidden());
    proxyModel_->setSortCaseSensitivity(folderSettings.sortCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    proxyModel_->setSourceModel(folderModel_);
    // set view mode
    setViewMode(folderSettings.viewMode());

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

void TabPage::deselectAll() {
    folderView_->selectionModel()->clearSelection();
}

void TabPage::invertSelection() {
    folderView_->invertSelection();
}

void TabPage::reload() {
    if(folder_) {
        // don't select or scroll to the previous folder after reload
        lastFolderPath_ = Fm::FilePath();
        // but remember the current scroll position
        BrowseHistoryItem& item = history_.currentItem();
        QAbstractItemView* childView = folderView_->childView();
        item.setScrollPos(childView->verticalScrollBar()->value());

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
        if(numSel == 1) { /* only one file is selected (also, tell if it is a symlink)*/
            auto& fi = files.front();
            if(!fi->isDir()) {
                QString name = static_cast<Application*>(qApp)->settings().showFullNames()
                               && strcmp(fi->dirPath().uriScheme().get(), "menu") != 0
                                   ? QString::fromStdString(fi->name())
                                   : fi->displayName();
                if(fi->isSymlink()) {
                    msg = QStringLiteral("\"%1\" (%2) %3 (%4)")
                          .arg(name,
                          Fm::formatFileSize(fi->size(), fm_config->si_unit),
                          QString::fromUtf8(fi->mimeType()->desc()),
                          tr("Link to") + QChar(QChar::Space) + QString::fromStdString(fi->target()));
                }
                else {
                    msg = QStringLiteral("\"%1\" (%2) %3")
                          .arg(name,
                          Fm::formatFileSize(fi->size(), fm_config->si_unit), // FIXME: deprecate fm_config
                          QString::fromUtf8(fi->mimeType()->desc()));
                }
            }
            else {
                if(fi->isSymlink()) {
                    msg = QStringLiteral("\"%1\" %2 (%3)")
                          .arg(fi->displayName(),
                          QString::fromUtf8(fi->mimeType()->desc()),
                          tr("Link to") + QChar(QChar::Space) + QString::fromStdString(fi->target()));
                }
                else {
                    msg = QStringLiteral("\"%1\" %2")
                          .arg(fi->displayName(),
                          QString::fromUtf8(fi->mimeType()->desc()));
                }
            }
            /* FIXME: should we support statusbar plugins as in the gtk+ version? */
        }
        else {
            goffset sum;
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
                    msg += QStringLiteral(" (%1)").arg(Fm::formatFileSize(sum, fm_config->si_unit)); // FIXME: deprecate fm_config
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
    if(index >= 0 && static_cast<size_t>(index) < history_.size()) {
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
    if(settings.noItemTooltip()) {
        folderView_->childView()->viewport()->removeEventFilter(this);
        folderView_->childView()->viewport()->installEventFilter(this);
    }
    else {
        folderView_->childView()->viewport()->removeEventFilter(this);
    }
}

void TabPage::setViewMode(Fm::FolderView::ViewMode mode) {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    if(folderSettings_.viewMode() != mode) {
        folderSettings_.setViewMode(mode);
        if(folderSettings_.isCustomized()) {
            settings.saveFolderSettings(path(), folderSettings_);
        }
    }
    Fm::FolderView::ViewMode prevMode = folderView_->viewMode();
    folderView_->setViewMode(mode);
    if(folderView_->isVisible()) { // in the current tab
        folderView_->childView()->setFocus();
    }
    if(prevMode != folderView_->viewMode()) {
        // FolderView::setViewMode() may delete the view to switch between list and tree.
        // So, the event filter should be re-installed and the status message should be updated.
        folderView_->childView()->removeEventFilter(this);
        folderView_->childView()->installEventFilter(this);
        if(settings.noItemTooltip()) {
            folderView_->childView()->viewport()->removeEventFilter(this);
            folderView_->childView()->viewport()->installEventFilter(this);
        }
        onSelChanged();
    }
}

void TabPage::sort(int col, Qt::SortOrder order) {
    if(proxyModel_) {
        proxyModel_->sort(col, order);
    }
}

void TabPage::setSortFolderFirst(bool value) {
    if(proxyModel_) {
        proxyModel_->setFolderFirst(value);
    }
}

void TabPage::setSortHiddenLast(bool value) {
    if(proxyModel_) {
        proxyModel_->setHiddenLast(value);
    }
}

void TabPage::setSortCaseSensitive(bool value) {
    if(proxyModel_) {
        proxyModel_->setSortCaseSensitivity(value ? Qt::CaseSensitive : Qt::CaseInsensitive);
    }
}

void TabPage::setShowHidden(bool showHidden) {
    if(proxyModel_) {
        proxyModel_->setShowHidden(showHidden);
    }
}

void TabPage::setShowThumbnails(bool showThumbnails) {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    settings.setShowThumbnails(showThumbnails);
    if(proxyModel_) {
        proxyModel_->setShowThumbnails(showThumbnails);
    }
}

void TabPage::saveFolderSorting() {
    if (proxyModel_ == nullptr) {
        return;
    }
    folderSettings_.setSortOrder(proxyModel_->sortOrder());
    folderSettings_.setSortColumn(static_cast<Fm::FolderModel::ColumnId>(proxyModel_->sortColumn()));
    folderSettings_.setSortFolderFirst(proxyModel_->folderFirst());
    folderSettings_.setSortHiddenLast(proxyModel_->hiddenLast());
    folderSettings_.setSortCaseSensitive(proxyModel_->sortCaseSensitivity());
    if(folderSettings_.showHidden() != proxyModel_->showHidden()) {
        folderSettings_.setShowHidden(proxyModel_->showHidden());
        statusText_[StatusTextNormal] = formatStatusText();
        Q_EMIT statusChanged(StatusTextNormal, statusText_[StatusTextNormal]);
    }
    if(folderSettings_.isCustomized()) {
        static_cast<Application*>(qApp)->settings().saveFolderSettings(path(), folderSettings_);
    }
}

void TabPage::applyFilter() {
    if(proxyModel_ == nullptr) {
        return;
    }

    int prevSelSize = folderView_->selectionModel()->selectedIndexes().size();

    proxyModel_->updateFilters();

    QModelIndex firstIndx = proxyModel_->index(0, 0);
    if(proxyFilter_->getFilterStr().isEmpty()) {
        QModelIndex curIndex = folderView_->selectionModel()->currentIndex();
        if (curIndex.isValid()) {
            // scroll to the current item on removing filter
            folderView_->childView()->scrollTo(curIndex, QAbstractItemView::EnsureVisible);
        }
        else if (firstIndx.isValid()) {
            // if there is no current item, set the first item as current without changing the selection
            folderView_->selectionModel()->setCurrentIndex(firstIndx, QItemSelectionModel::NoUpdate);
        }
    }
    else {
        bool selectionMade = false;
        if(firstIndx.isValid()
           && !static_cast<Application*>(qApp)->settings().showFilter()) {
            // preselect an appropriate item if the filter-bar is transient
            auto indexList = proxyModel_->match(firstIndx, Qt::DisplayRole, proxyFilter_->getFilterStr());
            if(!indexList.isEmpty()) {
                if(!folderView_->selectionModel()->isSelected(indexList.at(0))) {
                    folderView_->childView()->setCurrentIndex(indexList.at(0));
                    selectionMade = true;
                }
            }
            else if(!folderView_->selectionModel()->isSelected(firstIndx)) {
                folderView_->childView()->setCurrentIndex(firstIndx);
                selectionMade = true;
            }
        }

        // if no new selection is made and some selected files are filtered out,
        // "View::selChanged()" won't be emitted
        if(!selectionMade
           && prevSelSize > folderView_->selectionModel()->selectedIndexes().size()) {
                onSelChanged();
        }

        // ensure that the current item exists and is visible
        QModelIndex curIndex = folderView_->selectionModel()->currentIndex();
        if (curIndex.isValid()) {
            folderView_->childView()->scrollTo(curIndex, QAbstractItemView::EnsureVisible);
        }
        else {
            QModelIndexList selIndexes = folderView_->selectionModel()->selectedIndexes();
            curIndex = selIndexes.isEmpty() ? firstIndx : selIndexes.last();
            if (curIndex.isValid()) {
                folderView_->selectionModel()->setCurrentIndex(curIndex, QItemSelectionModel::NoUpdate);
                folderView_->childView()->scrollTo(curIndex, QAbstractItemView::EnsureVisible);
            }
        }
    }

    statusText_[StatusTextNormal] = formatStatusText();
    Q_EMIT statusChanged(StatusTextNormal, statusText_[StatusTextNormal]);
}

void TabPage::setCustomizedView(bool value, bool recursive) {
    if(folderSettings_.isCustomized() == value && folderSettings_.recursive() == recursive) {
        return;
    }

    Settings& settings = static_cast<Application*>(qApp)->settings();
    if(value) { // save customized folder view settings
        folderSettings_.setCustomized(value);
        folderSettings_.setRecursive(recursive);
        settings.saveFolderSettings(path(), folderSettings_);
    }
    else { // use default or inherited folder view settings
        settings.clearFolderSettings(path());
        // get folderSettings_ again because, although it isn't customized, it may be inherited
        folderSettings_ = settings.loadFolderSettings(path());
        // settings may change temporarily by connecting to the signal TabPage::sortFilterChanged,
        // which will be emitted below (that happens in MainWindow::onTabPageSortFilterChanged,
        // for example), so we should remember its relevant values before proceeding
        bool showHidden = folderSettings_.showHidden();
        bool sortCaseSensitive = folderSettings_.sortCaseSensitive();
        bool sortFolderFirst = folderSettings_.sortFolderFirst();
        bool sortHiddenLast = folderSettings_.sortHiddenLast();
        Fm::FolderModel::ColumnId sortColumn = folderSettings_.sortColumn();
        Qt::SortOrder sortOrder = folderSettings_.sortOrder();
        Fm::FolderView::ViewMode viewMode = folderSettings_.viewMode();
        setShowHidden(showHidden);
        setSortCaseSensitive(sortCaseSensitive);
        setSortFolderFirst(sortFolderFirst);
        setSortHiddenLast(sortHiddenLast);
        sort(sortColumn, sortOrder);
        setViewMode(viewMode);
    }
}

void TabPage::goToCustomizedViewSource() {
    if(const auto inheritedPath = folderSettings_.inheritedPath()) {
        chdir(inheritedPath);
    }
}

void TabPage::createShortcut() {
    if(folder_ && folder_->isLoaded()) {
        auto folderPath = folder_->path();
        if(folderPath && folderPath.isNative()) {
            DesktopEntryDialog* dlg = new DesktopEntryDialog(this, folderPath);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            connect(dlg, &DesktopEntryDialog::desktopEntryCreated, [this] (const Fm::FilePath& parent, const QString& name) {
                // if the current directory does not have a file monitor or is changed,
                // there will be no point to tracking the created shortcut
                if(folder_ && folder_->hasFileMonitor() && folder_->path() == parent) {
                    filesToTrust_ << name;
                }
            });
            dlg->show();
            if(!static_cast<Application*>(qApp)->underWayland()) {
                dlg->raise();
                dlg->activateWindow();
            }
        }
    }
}

bool TabPage::canOpenAdmin() {
    /* NOTE: "admin:///" requires a special handling because it first needs an invisible mount
       and then the password. Since its password prompt is shown with all GIO functions that
       query file info, a direct call to chdir() will show two password prompts if the first one
       is cancelled.

       Fm::uriExists() (= g_file_query_exists) is used to check Admin. It calls the password prompt
       if Admin is mounted. Four scenarios are possible:

       1. Admin is not supported. Then, the mount operation fails and this method returns "false"
          after showing an error message.

       2. Admin is supported but not mounted yet. Then, the first call to Fm::uriExists()
          does not show a password prompt. We mount Admin and ask for the password by calling
          Fm::uriExists() again.

       3. If Admin is already mounted but the correct password is not entered yet, the password
          will be asked by the first call to Fm::uriExists(). If the password is correct, "true"
          will be returned; if not, MountOperation::wait() will return "false" (because a repeated
          mount fails) and so, another password prompt will not be shown.

       4. If Admin is already mounted and the password was entered before, "true" will be returned.
    */
    const char* admin = "admin:///";
    if(Fm::uriExists(admin)) {
        return true;
    }
    MountOperation* op = new MountOperation(false);
    op->mountEnclosingVolume(Fm::FilePath::fromUri(admin));
    if(op->wait() && Fm::uriExists(admin)) {
        return true;
    }
    QMessageBox::critical(parentWidget()->window(), QObject::tr("Error"), QObject::tr("Cannot open as Admin."));
    return false;
}

} // namespace PCManFM
