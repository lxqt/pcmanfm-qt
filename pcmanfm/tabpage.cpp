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
#include <QApplication>
#include <QCursor>
#include <QMessageBox>
#include <QScrollBar>
#include <libfm-qt/proxyfoldermodel.h>
#include "settings.h"
#include "application.h"
#include <libfm-qt/cachedfoldermodel.h>
#include <QTimer>
#include <QTextStream>

using namespace Fm;

namespace PCManFM {

bool ProxyFilter::filterAcceptsRow(const Fm::ProxyFolderModel* model, FmFileInfo* info) const {
  if(!model || !info)
    return true;
  QString baseName(fm_file_info_get_name(info));
  if(!virtHiddenList_.isEmpty() && !model->showHidden() && virtHiddenList_.contains(baseName))
      return false;
  if(!filterStr_.isEmpty() && !baseName.contains(filterStr_, Qt::CaseInsensitive))
    return false;
  return true;
}

void ProxyFilter::setVirtHidden(FmFolder* folder) {
  virtHiddenList_ = QStringList(); // reset the list
  if(!folder) return;
  if(FmPath* path = fm_folder_get_path(folder)) {
    char* pathStr = fm_path_to_str(path);
    if(pathStr) {
      QString dotHidden = QString(pathStr) + QString("/.hidden");
      g_free(pathStr);
      QFile file(dotHidden);
      if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while(!in.atEnd())
          virtHiddenList_.append(in.readLine());
        file.close();
      }
    }
  }
}

TabPage::TabPage(FmPath* path, QWidget* parent):
  QWidget(parent),
  folderModel_(NULL),
  folder_(NULL),
  overrideCursor_(false) {

  Settings& settings = static_cast<Application*>(qApp)->settings();

  // create proxy folder model to do item filtering
  proxyModel_ = new ProxyFolderModel();
  proxyModel_->setShowHidden(settings.showHidden());
  proxyModel_->setShowThumbnails(settings.showThumbnails());
  connect(proxyModel_, &ProxyFolderModel::sortFilterChanged, this, &TabPage::onModelSortFilterChanged);

  verticalLayout = new QVBoxLayout(this);
  verticalLayout->setContentsMargins(0, 0, 0, 0);

  folderView_ = new View(settings.viewMode(), this);
  folderView_->setMargins(settings.folderViewCellMargins());
  // newView->setColumnWidth(Fm::FolderModel::ColumnName, 200);
  connect(folderView_, &View::openDirRequested, this, &TabPage::onOpenDirRequested);
  connect(folderView_, &View::selChanged, this, &TabPage::onSelChanged);
  connect(folderView_, &View::clickedBack, this, &TabPage::backwardRequested);
  connect(folderView_, &View::clickedForward, this, &TabPage::forwardRequested);

  proxyFilter_ = new ProxyFilter();
  proxyModel_->addFilter(proxyFilter_);

  // FIXME: this is very dirty
  folderView_->setModel(proxyModel_);
  verticalLayout->addWidget(folderView_);

  chdir(path, true);
}

TabPage::~TabPage() {
  freeFolder();
  if(proxyFilter_)
    delete proxyFilter_;
  if(proxyModel_)
    delete proxyModel_;
  if(folderModel_)
    folderModel_->unref();

  if(overrideCursor_) {
    QApplication::restoreOverrideCursor(); // remove busy cursor
  }
}

void TabPage::freeFolder() {
  if(folder_) {
    g_signal_handlers_disconnect_by_func(folder_, (gpointer)onFolderStartLoading, this);
    g_signal_handlers_disconnect_by_func(folder_, (gpointer)onFolderFinishLoading, this);
    g_signal_handlers_disconnect_by_func(folder_, (gpointer)onFolderError, this);
    g_signal_handlers_disconnect_by_func(folder_, (gpointer)onFolderFsInfo, this);
    g_signal_handlers_disconnect_by_func(folder_, (gpointer)onFolderRemoved, this);
    g_signal_handlers_disconnect_by_func(folder_, (gpointer)onFolderUnmount, this);
    g_signal_handlers_disconnect_by_func(folder_, (gpointer)onFolderContentChanged, this);
    g_object_unref(folder_);
    folder_ = NULL;
  }
}

/*static*/ void TabPage::onFolderStartLoading(FmFolder* _folder, TabPage* pThis) {
  if(!pThis->overrideCursor_) {
    // FIXME: sometimes FmFolder of libfm generates unpaired "start-loading" and
    // "finish-loading" signals of uncertain reasons. This should be a bug in libfm.
    // Until it's fixed in libfm, we need to workaround the problem here, not to
    // override the cursor twice.
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    pThis->overrideCursor_ = true;
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
    fm_folder_view_set_model(fv, NULL);
#endif
}

// slot
void TabPage::restoreScrollPos() {
  // scroll to recorded position
  folderView_->childView()->verticalScrollBar()->setValue(browseHistory().currentScrollPos());

  // if the current folder is the parent folder of the last browsed folder,
  // select the folder item in current view.
  if(lastFolderPath_.parent() == path()) {
    QModelIndex index = folderView_->indexFromFolderPath(lastFolderPath_.data());
    if(index.isValid()) {
      folderView_->childView()->scrollTo(index, QAbstractItemView::EnsureVisible);
      folderView_->childView()->setCurrentIndex(index);
    }
  }
}

/*static*/ void TabPage::onFolderFinishLoading(FmFolder* _folder, TabPage* pThis) {
  // FIXME: is this needed?
  FmFileInfo* fi = fm_folder_get_info(_folder);
  if(fi) { // if loading of the folder fails, it's possible that we don't have FmFileInfo.
    pThis->title_ = QString::fromUtf8(fm_file_info_get_disp_name(fi));
    Q_EMIT pThis->titleChanged(pThis->title_);
  }

  fm_folder_query_filesystem_info(_folder); // FIXME: is this needed?
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
  if(fm_folder_view_get_model(fv) == NULL) {
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
  QString& text = pThis->statusText_[StatusTextNormal];
  text = pThis->formatStatusText();
  Q_EMIT pThis->statusChanged(StatusTextNormal, text);

  if(pThis->overrideCursor_) {
    QApplication::restoreOverrideCursor(); // remove busy cursor
    pThis->overrideCursor_ = false;
  }

  // After finishing loading the folder, the model is updated, but Qt delays the UI update
  // for performance reasons. Therefore at this point the UI is not up to date.
  // Of course, the scrollbar ranges are not updated yet. We solve this by installing an Qt timeout handler.
  QTimer::singleShot(10, pThis, SLOT(restoreScrollPos()));
}

/*static*/ FmJobErrorAction TabPage::onFolderError(FmFolder* _folder, GError* err, FmJobErrorSeverity severity, TabPage* pThis) {
  if(err->domain == G_IO_ERROR) {
    if(err->code == G_IO_ERROR_NOT_MOUNTED && severity < FM_JOB_ERROR_CRITICAL) {
      FmPath* path = fm_folder_get_path(_folder);
      MountOperation* op = new MountOperation(pThis);
      op->mount(path);
      if(op->wait()) { // blocking event loop, wait for mount operation to finish.
        // This will reload the folder, which generates a new "start-loading"
        // signal, so we get more "start-loading" signals than "finish-loading"
        // signals. FIXME: This is a bug of libfm.
        // Because the two signals are not correctly paired, we need to
        // remove busy cursor here since "finish-loading" is not emitted.
        QApplication::restoreOverrideCursor(); // remove busy cursor
        pThis->overrideCursor_ = false;
        return FM_JOB_RETRY;
      }
    }
  }
  if(severity >= FM_JOB_ERROR_MODERATE) {
    /* Only show more severe errors to the users and
      * ignore milder errors. Otherwise too many error
      * message boxes can be annoying.
      * This fixes bug #3411298- Show "Permission denied" when switching to super user mode.
      * https://sourceforge.net/tracker/?func=detail&aid=3411298&group_id=156956&atid=801864
      * */

    // FIXME: consider replacing this modal dialog with an info bar to improve usability
    QMessageBox::critical(pThis, tr("Error"), QString::fromUtf8(err->message));
  }
  return FM_JOB_CONTINUE;
}

/*static*/ void TabPage::onFolderFsInfo(FmFolder* _folder, TabPage* pThis) {
  guint64 free, total;
  QString& msg = pThis->statusText_[StatusTextFSInfo];
  if(fm_folder_get_filesystem_info(_folder, &total, &free)) {
    char total_str[64];
    char free_str[64];
    fm_file_size_to_str(free_str, sizeof(free_str), free, fm_config->si_unit);
    fm_file_size_to_str(total_str, sizeof(total_str), total, fm_config->si_unit);
    msg = tr("Free space: %1 (Total: %2)")
          .arg(QString::fromUtf8(free_str))
          .arg(QString::fromUtf8(total_str));
  }
  else
    msg.clear();
  Q_EMIT pThis->statusChanged(StatusTextFSInfo, msg);
}

QString TabPage::formatStatusText() {
  if(proxyModel_ && folder_) {
    FmFileInfoList* files = fm_folder_get_files(folder_);
    int total_files = fm_file_info_list_get_length(files);
    int shown_files = proxyModel_->rowCount();
    int hidden_files = total_files - shown_files;
    QString text = tr("%n item(s)", "", shown_files);
    if(hidden_files > 0)
      text += tr(" (%n hidden)", "", hidden_files);
    return text;
  }
  return QString();
}

/*static*/ void TabPage::onFolderRemoved(FmFolder* _folder, TabPage* pThis) {
  // the folder we're showing is removed, destroy the widget
  qDebug("folder removed");
  Settings& settings = static_cast<Application*>(qApp)->settings();
  // NOTE: call pThis->deleteLater() directly from this GObject signal handler
  // does not work but I don't know why.
  // Maybe it's the problem of glib mainloop integration?
  // Call it when idle works, though.
  if(settings.closeOnUnmount())
    QTimer::singleShot(0, pThis, SLOT(deleteLater()));
  else
    pThis->chdir(fm_path_get_home());
}

/*static*/ void TabPage::onFolderUnmount(FmFolder* _folder, TabPage* pThis) {
  // the folder we're showing is unmounted, destroy the widget
  qDebug("folder unmount");
  // NOTE: call pThis->deleteLater() directly from this GObject signal handler
  // does not work but I don't know why.
  // Maybe it's the problem of glib mainloop integration?
  // Call it when idle works, though.
  Settings& settings = static_cast<Application*>(qApp)->settings();
  // NOTE: call pThis->deleteLater() directly from this GObject signal handler
  // does not work but I don't know why.
  // Maybe it's the problem of glib mainloop integration?
  // Call it when idle works, though.
  if(settings.closeOnUnmount())
    QTimer::singleShot(0, pThis, SLOT(deleteLater()));
  else
    pThis->chdir(fm_path_get_home());
}

/*static */ void TabPage::onFolderContentChanged(FmFolder* _folder, TabPage* pThis) {
  /* update status text */
  pThis->statusText_[StatusTextNormal] = pThis->formatStatusText();
  Q_EMIT pThis->statusChanged(StatusTextNormal, pThis->statusText_[StatusTextNormal]);
}

QString TabPage::pathName() {
  char* disp_path = fm_path_display_name(path(), TRUE);
  QString ret = QString::fromUtf8(disp_path);
  g_free(disp_path);
  return ret;
}

void TabPage::chdir(FmPath* newPath, bool addHistory) {
  if(folder_) {
    // we're already in the specified dir
    if(fm_path_equal(newPath, fm_folder_get_path(folder_)))
      return;

    // remember the previous folder path that we have browsed.
    lastFolderPath_ = fm_folder_get_path(folder_);

    if(addHistory) {
      // store current scroll pos in the browse history
      BrowseHistoryItem& item = history_.currentItem();
      QAbstractItemView* childView = folderView_->childView();
      item.setScrollPos(childView->verticalScrollBar()->value());
    }

    // free the previous model
    if(folderModel_) {
      proxyModel_->setSourceModel(NULL);
      folderModel_->unref(); // unref the cached model
      folderModel_ = NULL;
    }

    freeFolder();
  }

  char* disp_name = fm_path_display_basename(newPath);
  title_ = QString::fromUtf8(disp_name);
  Q_EMIT titleChanged(title_);
  g_free(disp_name);

  folder_ = fm_folder_from_path(newPath);
  proxyFilter_->setVirtHidden(folder_);
  if(addHistory) {
    // add current path to browse history
    history_.add(path());
  }
  g_signal_connect(folder_, "start-loading", G_CALLBACK(onFolderStartLoading), this);
  g_signal_connect(folder_, "finish-loading", G_CALLBACK(onFolderFinishLoading), this);
  g_signal_connect(folder_, "error", G_CALLBACK(onFolderError), this);
  g_signal_connect(folder_, "fs-info", G_CALLBACK(onFolderFsInfo), this);
  /* destroy the page when the folder is unmounted or deleted. */
  g_signal_connect(folder_, "removed", G_CALLBACK(onFolderRemoved), this);
  g_signal_connect(folder_, "unmount", G_CALLBACK(onFolderUnmount), this);
  g_signal_connect(folder_, "content-changed", G_CALLBACK(onFolderContentChanged), this);

  folderModel_ = CachedFolderModel::modelFromFolder(folder_);
  proxyModel_->setSourceModel(folderModel_);
  proxyModel_->sort(proxyModel_->sortColumn(), proxyModel_->sortOrder());
  Settings& settings = static_cast<Application*>(qApp)->settings();
  proxyModel_->setFolderFirst(settings.sortFolderFirst());
  proxyModel_->sort(settings.sortColumn(), settings.sortOrder());

  if(fm_folder_is_loaded(folder_)) {
    onFolderStartLoading(folder_, this);
    onFolderFinishLoading(folder_, this);
    onFolderFsInfo(folder_, this);
  }
  else
    onFolderStartLoading(folder_, this);
}

void TabPage::selectAll() {
  folderView_->selectAll();
}

void TabPage::invertSelection() {
  folderView_->invertSelection();
}

void TabPage::onOpenDirRequested(FmPath* path, int target) {
  Q_EMIT openDirRequested(path, target);
}

// when the current selection in the folder view is changed
void TabPage::onSelChanged(int numSel) {
  QString msg;
  if(numSel > 0) {
    /* FIXME: display total size of all selected files. */
    if(numSel == 1) { /* only one file is selected */
      FmFileInfoList* files = folderView_->selectedFiles();
      FmFileInfo* fi = fm_file_info_list_peek_head(files);
      const char* size_str = fm_file_info_get_disp_size(fi);
      if(size_str) {
        msg = QString("\"%1\" (%2) %3")
                        .arg(QString::fromUtf8(fm_file_info_get_disp_name(fi)))
                        .arg(QString::fromUtf8(size_str ? size_str : ""))
                        .arg(QString::fromUtf8(fm_file_info_get_desc(fi)));
      }
      else {
        msg = QString("\"%1\" %2")
                        .arg(QString::fromUtf8(fm_file_info_get_disp_name(fi)))
                        .arg(QString::fromUtf8(fm_file_info_get_desc(fi)));
      }
      /* FIXME: should we support statusbar plugins as in the gtk+ version? */
      fm_file_info_list_unref(files);
    }
    else {
      goffset sum;
      GList* l;
      msg = tr("%n item(s) selected", nullptr, numSel).arg(numSel);
      /* don't count if too many files are selected, that isn't lightweight */
      if(numSel < 1000) {
        sum = 0;
        FmFileInfoList* files = folderView_->selectedFiles();
        for(l = fm_file_info_list_peek_head_link(files); l; l = l->next) {
          if(fm_file_info_is_dir(FM_FILE_INFO(l->data))) {
            /* if we got a directory then we cannot tell it's size
            unless we do deep count but we cannot afford it */
            sum = -1;
            break;
          }
          sum += fm_file_info_get_size(FM_FILE_INFO(l->data));
        }
        if(sum >= 0) {
          char size_str[128];
          fm_file_size_to_str(size_str, sizeof(size_str), sum,
                              fm_config->si_unit);
	  msg += QString(" (%1)").arg(QString::fromUtf8(size_str));
        }
      /* FIXME: should we support statusbar plugins as in the gtk+ version? */
        fm_file_info_list_unref(files);
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

void TabPage::jumpToHistory(int index)
{
  if(index >=0 && index < history_.size()) {
    // remember current scroll position
    BrowseHistoryItem& item = history_.currentItem();
    QAbstractItemView* childView = folderView_->childView();
    item.setScrollPos(childView->verticalScrollBar()->value());

    history_.setCurrentIndex(index);
    chdir(history_.currentPath(), false);
  }
}

bool TabPage::canUp() {
  return (path() != NULL && fm_path_get_parent(path()) != NULL);
}

void TabPage::up() {
  FmPath* _path = path();
  if(_path) {
    FmPath* parent = fm_path_get_parent(_path);
    if(parent) {
      chdir(parent, true);
    }
  }
}

void TabPage::onModelSortFilterChanged() {
  Q_EMIT sortFilterChanged();
}

void TabPage::updateFromSettings(Settings& settings) {
  folderView_->updateFromSettings(settings);
}

void TabPage::setShowHidden(bool showHidden) {
  if(!proxyModel_ || showHidden == proxyModel_->showHidden())
    return;
  proxyModel_->setShowHidden(showHidden);
  statusText_[StatusTextNormal] = formatStatusText();
  Q_EMIT statusChanged(StatusTextNormal, statusText_[StatusTextNormal]);
}

void TabPage:: applyFilter() {
  if(!proxyModel_) return;
  proxyModel_->updateFilters();
  statusText_[StatusTextNormal] = formatStatusText();
  Q_EMIT statusChanged(StatusTextNormal, statusText_[StatusTextNormal]);
}

} // namespace PCManFM
