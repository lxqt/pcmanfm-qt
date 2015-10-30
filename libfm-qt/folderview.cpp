/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "folderview.h"
#include "foldermodel.h"
#include <QHeaderView>
#include <QVBoxLayout>
#include <QContextMenuEvent>
#include "proxyfoldermodel.h"
#include "folderitemdelegate.h"
#include "dndactionmenu.h"
#include "filemenu.h"
#include "foldermenu.h"
#include "filelauncher.h"
#include <QTimer>
#include <QDate>
#include <QDebug>
#include <QMimeData>
#include <QHoverEvent>
#include <QApplication>
#include <QScrollBar>
#include <QMetaType>
#include "folderview_p.h"

Q_DECLARE_OPAQUE_POINTER(FmFileInfo*)

namespace Fm {

FolderViewListView::FolderViewListView(QWidget* parent):
  QListView(parent),
  activationAllowed_(true) {
  connect(this, &QListView::activated, this, &FolderViewListView::activation);
}

FolderViewListView::~FolderViewListView() {
}

void FolderViewListView::startDrag(Qt::DropActions supportedActions) {
  if(movement() != Static)
    QListView::startDrag(supportedActions);
  else
    QAbstractItemView::startDrag(supportedActions);
}

void FolderViewListView::mousePressEvent(QMouseEvent* event) {
  QListView::mousePressEvent(event);
  static_cast<FolderView*>(parent())->childMousePressEvent(event);
}

QModelIndex FolderViewListView::indexAt(const QPoint& point) const {
  QModelIndex index = QListView::indexAt(point);
  // NOTE: QListView has a severe design flaw here. It does hit-testing based on the
  // total bound rect of the item. The width of an item is determined by max(icon_width, text_width).
  // So if the text label is much wider than the icon, when you click outside the icon but
  // the point is still within the outer bound rect, the item is still selected.
  // This results in very poor usability. Let's do precise hit-testing here.
  // An item is hit only when the point is in the icon or text label.
  // If the point is in the bound rectangle but outside the icon or text, it should not be selected.
  if(viewMode() == QListView::IconMode && index.isValid()) {
    // FIXME: this hack only improves the usability partially. We still need more precise sizeHint handling.
    // FolderItemDelegate* delegate = static_cast<FolderItemDelegate*>(itemDelegateForColumn(FolderModel::ColumnFileName));
    // Q_ASSERT(delegate != NULL);
    // We use the grid size - (2, 2) as the size of the bounding rectangle of the whole item.
    // The width of the text label hence is gridSize.width - 2, and the width and height of the icon is from iconSize().
    QRect visRect = visualRect(index); // visibal area on the screen
    QSize itemSize = gridSize();
    itemSize.setWidth(itemSize.width() - 2);
    itemSize.setHeight(itemSize.height() - 2);
    QSize _iconSize = iconSize();
    int textHeight = itemSize.height() - _iconSize.height();
    if(point.y() < visRect.bottom() - textHeight) {
      // the point is in the icon area, not over the text label
      int iconXMargin = (itemSize.width() - _iconSize.width()) / 2;
      if(point.x() < (visRect.left() + iconXMargin) || point.x() > (visRect.right() - iconXMargin))
	return QModelIndex();
    }
    // qDebug() << "visualRect: " << visRect << "point:" << point;
  }
  return index;
}


// NOTE:
// QListView has a problem which I consider a bug or a design flaw.
// When you set movement property to Static, theoratically the icons
// should not be movable. However, if you turned on icon mode,
// the icons becomes freely movable despite the value of movement is Static.
// To overcome this bug, we override all drag handling methods, and
// call QAbstractItemView directly, bypassing QListView.
// In this way, we can workaround the buggy behavior.
// The drag handlers of QListView basically does the same things
// as its parent QAbstractItemView, but it also stores the currently
// dragged item and paint them in the view as needed.
// TODO: I really should file a bug report to Qt developers.

void FolderViewListView::dragEnterEvent(QDragEnterEvent* event) {
  if(movement() != Static)
    QListView::dragEnterEvent(event);
  else
    QAbstractItemView::dragEnterEvent(event);
  qDebug("dragEnterEvent");
  //static_cast<FolderView*>(parent())->childDragEnterEvent(event);
}

void FolderViewListView::dragLeaveEvent(QDragLeaveEvent* e) {
  if(movement() != Static)
    QListView::dragLeaveEvent(e);
  else
    QAbstractItemView::dragLeaveEvent(e);
  static_cast<FolderView*>(parent())->childDragLeaveEvent(e);
}

void FolderViewListView::dragMoveEvent(QDragMoveEvent* e) {
  if(movement() != Static)
    QListView::dragMoveEvent(e);
  else
    QAbstractItemView::dragMoveEvent(e);
  static_cast<FolderView*>(parent())->childDragMoveEvent(e);
}

void FolderViewListView::dropEvent(QDropEvent* e) {

  static_cast<FolderView*>(parent())->childDropEvent(e);

  if(movement() != Static)
    QListView::dropEvent(e);
  else
    QAbstractItemView::dropEvent(e);
}

void FolderViewListView::mouseReleaseEvent(QMouseEvent* event) {
  bool activationWasAllowed = activationAllowed_;
  if ((!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, NULL, this)) || (event->button() != Qt::LeftButton)) {
    activationAllowed_ = false;
  }

  QListView::mouseReleaseEvent(event);

  activationAllowed_ = activationWasAllowed;
}

void FolderViewListView::mouseDoubleClickEvent(QMouseEvent* event) {
  bool activationWasAllowed = activationAllowed_;
  if ((style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, NULL, this)) || (event->button() != Qt::LeftButton)) {
    activationAllowed_ = false;
  }

  QListView::mouseDoubleClickEvent(event);

  activationAllowed_ = activationWasAllowed;
}

void FolderViewListView::activation(const QModelIndex &index) {
  if (activationAllowed_) {
    Q_EMIT activatedFiltered(index);
  }
}

//-----------------------------------------------------------------------------

FolderViewTreeView::FolderViewTreeView(QWidget* parent):
  QTreeView(parent),
  layoutTimer_(NULL),
  doingLayout_(false),
  activationAllowed_(true) {

  header()->setStretchLastSection(true);
  setIndentation(0);

  connect(this, &QTreeView::activated, this, &FolderViewTreeView::activation);
}

FolderViewTreeView::~FolderViewTreeView() {
  if(layoutTimer_)
    delete layoutTimer_;
}

void FolderViewTreeView::setModel(QAbstractItemModel* model) {
  QTreeView::setModel(model);
  layoutColumns();
  if(ProxyFolderModel* proxyModel = qobject_cast<ProxyFolderModel*>(model)) {
    connect(proxyModel, &ProxyFolderModel::sortFilterChanged, this, &FolderViewTreeView::onSortFilterChanged,
            Qt::UniqueConnection);
    onSortFilterChanged();
  }
}

void FolderViewTreeView::mousePressEvent(QMouseEvent* event) {
  QTreeView::mousePressEvent(event);
  static_cast<FolderView*>(parent())->childMousePressEvent(event);
}

void FolderViewTreeView::dragEnterEvent(QDragEnterEvent* event) {
  QTreeView::dragEnterEvent(event);
  static_cast<FolderView*>(parent())->childDragEnterEvent(event);
}

void FolderViewTreeView::dragLeaveEvent(QDragLeaveEvent* e) {
  QTreeView::dragLeaveEvent(e);
  static_cast<FolderView*>(parent())->childDragLeaveEvent(e);
}

void FolderViewTreeView::dragMoveEvent(QDragMoveEvent* e) {
  QTreeView::dragMoveEvent(e);
  static_cast<FolderView*>(parent())->childDragMoveEvent(e);
}

void FolderViewTreeView::dropEvent(QDropEvent* e) {
  static_cast<FolderView*>(parent())->childDropEvent(e);
  QTreeView::dropEvent(e);
}

// the default list mode of QListView handles column widths
// very badly (worse than gtk+) and it's not very flexible.
// so, let's handle column widths outselves.
void FolderViewTreeView::layoutColumns() {
  // qDebug("layoutColumns");
  if(!model())
    return;
  doingLayout_ = true;
  QHeaderView* headerView = header();
  // the width that's available for showing the columns.
  int availWidth = viewport()->contentsRect().width();
  int desiredWidth = 0;

  // get the width that every column want
  int numCols = headerView->count();
  if(numCols > 0) {
    int* widths = new int[numCols]; // array to store the widths every column needs
    int column;
    for(column = 0; column < numCols; ++column) {
      int columnId = headerView->logicalIndex(column);
      // get the size that the column needs
      widths[column] = sizeHintForColumn(columnId);
      // compute the total width needed
      desiredWidth += widths[column];
    }

    int filenameColumn = headerView->visualIndex(FolderModel::ColumnFileName);
    // if the total witdh we want exceeds the available space
    if(desiredWidth > availWidth) {
      // Compute the width available for the filename column
      int filenameAvailWidth = availWidth - desiredWidth + widths[filenameColumn];

      // Compute the minimum acceptable width for the filename column
      int filenameMinWidth = qMin(200, sizeHintForColumn(filenameColumn));

      if (filenameAvailWidth > filenameMinWidth) {
        // Shrink the filename column to the available width
        widths[filenameColumn] = filenameAvailWidth;
      }
      else {
        // Set the filename column to its minimum width
        widths[filenameColumn] = filenameMinWidth;
      }
    }
    else {
      // Fill the extra available space with the filename column
      widths[filenameColumn] += availWidth - desiredWidth;
    }

    // really do the resizing for every column
    for(int column = 0; column < numCols; ++column) {
      headerView->resizeSection(column, widths[column]);
    }
    delete []widths;
  }
  doingLayout_ = false;

  if(layoutTimer_) {
    delete layoutTimer_;
    layoutTimer_ = NULL;
  }
}

void FolderViewTreeView::resizeEvent(QResizeEvent* event) {
  QAbstractItemView::resizeEvent(event);
  // prevent endless recursion.
  // When manually resizing columns, at the point where a horizontal scroll
  // bar has to be inserted or removed, the vertical size changes, a resize
  // event  occurs and the column headers are flickering badly if the column
  // layout is modified at this point. Therefore only layout the columns if
  // the horizontal size changes.
  if(!doingLayout_ && event->size().width() != event->oldSize().width())
    layoutColumns(); // layoutColumns() also triggers resizeEvent
}

void FolderViewTreeView::rowsInserted(const QModelIndex& parent, int start, int end) {
  QTreeView::rowsInserted(parent, start, end);
  queueLayoutColumns();
}

void FolderViewTreeView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
  QTreeView::rowsAboutToBeRemoved(parent, start, end);
  queueLayoutColumns();
}

void FolderViewTreeView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) {
  QTreeView::dataChanged(topLeft, bottomRight);
  // FIXME: this will be very inefficient
  // queueLayoutColumns();
}

void FolderViewTreeView::reset() {
  // Sometimes when the content of the model is radically changed, Qt does reset()
  // on the model rather than doing large amount of insertion and deletion.
  // This is for performance reason so in this case rowsInserted() and rowsAboutToBeRemoved()
  // might not be called. Hence we also have to re-layout the columns when the model is reset.
  // This fixes bug #190
  // https://github.com/lxde/pcmanfm-qt/issues/190
  QTreeView::reset();
  queueLayoutColumns();
}

void FolderViewTreeView::queueLayoutColumns() {
  // qDebug("queueLayoutColumns");
  if(!layoutTimer_) {
    layoutTimer_ = new QTimer();
    layoutTimer_->setSingleShot(true);
    layoutTimer_->setInterval(0);
    connect(layoutTimer_, &QTimer::timeout, this, &FolderViewTreeView::layoutColumns);
  }
  layoutTimer_->start();
}

void FolderViewTreeView::mouseReleaseEvent(QMouseEvent* event) {
  bool activationWasAllowed = activationAllowed_;
  if ((!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, NULL, this)) || (event->button() != Qt::LeftButton)) {
    activationAllowed_ = false;
  }

  QTreeView::mouseReleaseEvent(event);

  activationAllowed_ = activationWasAllowed;
}

void FolderViewTreeView::mouseDoubleClickEvent(QMouseEvent* event) {
  bool activationWasAllowed = activationAllowed_;
  if ((style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, NULL, this)) || (event->button() != Qt::LeftButton)) {
    activationAllowed_ = false;
  }

  QTreeView::mouseDoubleClickEvent(event);

  activationAllowed_ = activationWasAllowed;
}

void FolderViewTreeView::activation(const QModelIndex &index) {
  if (activationAllowed_) {
    Q_EMIT activatedFiltered(index);
  }
}

void FolderViewTreeView::onSortFilterChanged() {
  if(QSortFilterProxyModel* proxyModel = qobject_cast<QSortFilterProxyModel*>(model())) {
    header()->setSortIndicatorShown(true);
    header()->setSortIndicator(proxyModel->sortColumn(), proxyModel->sortOrder());
    if (!isSortingEnabled()) {
      setSortingEnabled(true);
    }
  }
}


//-----------------------------------------------------------------------------

FolderView::FolderView(ViewMode _mode, QWidget* parent):
  QWidget(parent),
  view(NULL),
  mode((ViewMode)0),
  autoSelectionDelay_(600),
  autoSelectionTimer_(NULL),
  selChangedTimer_(NULL),
  fileLauncher_(NULL),
  model_(NULL) {

  iconSize_[IconMode - FirstViewMode] = QSize(48, 48);
  iconSize_[CompactMode - FirstViewMode] = QSize(24, 24);
  iconSize_[ThumbnailMode - FirstViewMode] = QSize(128, 128);
  iconSize_[DetailedListMode - FirstViewMode] = QSize(24, 24);

  QVBoxLayout* layout = new QVBoxLayout();
  layout->setMargin(0);
  setLayout(layout);

  setViewMode(_mode);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  connect(this, &FolderView::clicked, this, &FolderView::onFileClicked);
}

FolderView::~FolderView() {
  // qDebug("delete FolderView");
}

void FolderView::onItemActivated(QModelIndex index) {
  if(index.isValid() && index.model()) {
    QVariant data = index.model()->data(index, FolderModel::FileInfoRole);
    FmFileInfo* info = (FmFileInfo*)data.value<void*>();
    if(info) {
      if (!(QApplication::keyboardModifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
        Q_EMIT clicked(ActivatedClick, info);
      }
    }
  }
}

void FolderView::onSelChangedTimeout() {
  selChangedTimer_->deleteLater();
  selChangedTimer_ = NULL;

  QItemSelectionModel* selModel = selectionModel();
  int nSel = 0;
  if(viewMode() == DetailedListMode)
    nSel = selModel->selectedRows().count();
  else {
    nSel = selModel->selectedIndexes().count();
  }
  // qDebug()<<"selected:" << nSel;
  Q_EMIT selChanged(nSel); // FIXME: this is inefficient
}

void FolderView::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  // It's possible that the selected items change too often and this slot gets called for thousands of times.
  // For example, when you select thousands of files and delete them, we will get one selectionChanged() event
  // for every deleted file. So, we use a timer to delay the handling to avoid too frequent updates of the UI.
  if(!selChangedTimer_) {
    selChangedTimer_ = new QTimer(this);
    selChangedTimer_->setSingleShot(true);
    connect(selChangedTimer_, &QTimer::timeout, this, &FolderView::onSelChangedTimeout);
    selChangedTimer_->start(200);
  }
}


void FolderView::setViewMode(ViewMode _mode) {
  if(_mode == mode) // if it's the same more, ignore
    return;
  // FIXME: retain old selection

  // since only detailed list mode uses QTreeView, and others
  // all use QListView, it's wise to preserve QListView when possible.
  bool recreateView = false;
  if(view && (mode == DetailedListMode || _mode == DetailedListMode)) {
    delete view; // FIXME: no virtual dtor?
    view = NULL;
    recreateView = true;
  }
  mode = _mode;
  QSize iconSize = iconSize_[mode - FirstViewMode];

  if(mode == DetailedListMode) {
    FolderViewTreeView* treeView = new FolderViewTreeView(this);
    connect(treeView, &FolderViewTreeView::activatedFiltered, this, &FolderView::onItemActivated);

    view = treeView;
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    treeView->setAllColumnsShowFocus(false);

    // set our own custom delegate
    FolderItemDelegate* delegate = new FolderItemDelegate(treeView);
    treeView->setItemDelegateForColumn(FolderModel::ColumnFileName, delegate);
  }
  else {
    FolderViewListView* listView;
    if(view)
      listView = static_cast<FolderViewListView*>(view);
    else {
      listView = new FolderViewListView(this);
      connect(listView, &FolderViewListView::activatedFiltered, this, &FolderView::onItemActivated);
      view = listView;
    }
    // set our own custom delegate
    FolderItemDelegate* delegate = new FolderItemDelegate(listView);
    listView->setItemDelegateForColumn(FolderModel::ColumnFileName, delegate);
    // FIXME: should we expose the delegate?
    listView->setMovement(QListView::Static);
    listView->setResizeMode(QListView::Adjust);
    listView->setWrapping(true);
    switch(mode) {
      case IconMode: {
        listView->setViewMode(QListView::IconMode);
        listView->setWordWrap(true);
        listView->setFlow(QListView::LeftToRight);
        break;
      }
      case CompactMode: {
        listView->setViewMode(QListView::ListMode);
        listView->setWordWrap(false);
        listView->setFlow(QListView::QListView::TopToBottom);
        break;
      }
      case ThumbnailMode: {
        listView->setViewMode(QListView::IconMode);
        listView->setWordWrap(true);
        listView->setFlow(QListView::LeftToRight);
        break;
      }
      default:;
    }
    updateGridSize();
  }
  if(view) {
    // we have to install the event filter on the viewport instead of the view itself.
    view->viewport()->installEventFilter(this);
    // we want the QEvent::HoverMove event for single click + auto-selection support
    view->viewport()->setAttribute(Qt::WA_Hover, true);
    view->setContextMenuPolicy(Qt::NoContextMenu); // defer the context menu handling to parent widgets
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setIconSize(iconSize);

    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout()->addWidget(view);

    // enable dnd
    view->setDragEnabled(true);
    view->setAcceptDrops(true);
    view->setDragDropMode(QAbstractItemView::DragDrop);
    view->setDropIndicatorShown(true);

    if(model_) {
      // FIXME: preserve selections
      model_->setThumbnailSize(iconSize.width());
      view->setModel(model_);
      if(recreateView)
        connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FolderView::onSelectionChanged);
    }
  }
}

// set proper grid size for the QListView based on current view mode, icon size, and font size.
void FolderView::updateGridSize() {
  if(mode == DetailedListMode || !view)
    return;
  FolderViewListView* listView = static_cast<FolderViewListView*>(view);
  QSize icon = iconSize(mode); // size of the icon
  QFontMetrics fm = fontMetrics(); // size of current font
  QSize grid; // the final grid size
  switch(mode) {
    case IconMode:
    case ThumbnailMode: {
      // NOTE by PCMan about finding the optimal text label size:
      // The average filename length on my root filesystem is roughly 18-20 chars.
      // So, a reasonable size for the text label is about 10 chars each line since string of this length
      // can be shown in two lines. If you consider word wrap, then the result is around 10 chars per word.
      // In average, 10 char per line should be enough to display a "word" in the filename without breaking.
      // The values can be estimated with this command:
      // > find / | xargs  basename -a | sed -e s'/[_-]/ /g' | wc -mcw
      // However, this average only applies to English. For some Asian characters, such as Chinese chars,
      // each char actually takes doubled space. To be safe, we use 13 chars per line x average char width
      // to get a nearly optimal width for the text label. As most of the filenames have less than 40 chars
      // 13 chars x 3 lines should be enough to show the full filenames for most files.
      int textWidth = fm.averageCharWidth() * 12 + 4; // add 2 px padding for left and right border
      int textHeight = fm.height() * 3 + 4; // add 2 px padding for top and bottom border
      grid.setWidth(qMax(icon.width(), textWidth) + 8); // add a margin 4 px for every cell
      grid.setHeight(icon.height() + textHeight + 8); // add a margin 4 px for every cell
      break;
    }
    default:
      ; // do not use grid size
  }
  listView->setGridSize(grid);
  FolderItemDelegate* delegate = static_cast<FolderItemDelegate*>(listView->itemDelegateForColumn(FolderModel::ColumnFileName));
  delegate->setGridSize(grid);
}

void FolderView::setIconSize(ViewMode mode, QSize size) {
  Q_ASSERT(mode >= FirstViewMode && mode <= LastViewMode);
  iconSize_[mode - FirstViewMode] = size;
  if(viewMode() == mode) {
    view->setIconSize(size);
    if(model_)
      model_->setThumbnailSize(size.width());
    updateGridSize();
  }
}

QSize FolderView::iconSize(ViewMode mode) const {
  Q_ASSERT(mode >= FirstViewMode && mode <= LastViewMode);
  return iconSize_[mode - FirstViewMode];
}

FolderView::ViewMode FolderView::viewMode() const {
  return mode;
}

void FolderView::setAutoSelectionDelay(int delay) {
  autoSelectionDelay_ = delay;
}

QAbstractItemView* FolderView::childView() const {
  return view;
}

ProxyFolderModel* FolderView::model() const {
  return model_;
}

void FolderView::setModel(ProxyFolderModel* model) {
  if(view) {
    view->setModel(model);
    QSize iconSize = iconSize_[mode - FirstViewMode];
    model->setThumbnailSize(iconSize.width());
    if(view->selectionModel())
      connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FolderView::onSelectionChanged);
  }
  if(model_)
    delete model_;
  model_ = model;
}

bool FolderView::event(QEvent* event) {
  switch(event->type()) {
    case QEvent::StyleChange:
      break;
    case QEvent::FontChange:
      updateGridSize();
      break;
  };
  return QWidget::event(event);
}

void FolderView::contextMenuEvent(QContextMenuEvent* event) {
  QWidget::contextMenuEvent(event);
  QPoint pos = event->pos();
  QPoint view_pos = view->mapFromParent(pos);
  QPoint viewport_pos = view->viewport()->mapFromParent(view_pos);
  emitClickedAt(ContextMenuClick, viewport_pos);
}

void FolderView::childMousePressEvent(QMouseEvent* event) {
  // called from mousePressEvent() of child view
  Qt::MouseButton button = event->button();
  if(button == Qt::MiddleButton) {
    emitClickedAt(MiddleClick, event->pos());
  } else if (button == Qt::BackButton) {
    Q_EMIT clickedBack();
  } else if (button == Qt::ForwardButton) {
    Q_EMIT clickedForward();
  }
}

void FolderView::emitClickedAt(ClickType type, const QPoint& pos) {
  // indexAt() needs a point in "viewport" coordinates.
  QModelIndex index = view->indexAt(pos);
  if(index.isValid()) {
    QVariant data = index.data(FolderModel::FileInfoRole);
    FmFileInfo* info = reinterpret_cast<FmFileInfo*>(data.value<void*>());
    Q_EMIT clicked(type, info);
  }
  else {
    // FIXME: should we show popup menu for the selected files instead
    // if there are selected files?
    if(type == ContextMenuClick) {
      // clear current selection if clicked outside selected files
      view->clearSelection();
      Q_EMIT clicked(type, NULL);
    }
  }
}

QModelIndexList FolderView::selectedRows(int column) const {
  QItemSelectionModel* selModel = selectionModel();
  if(selModel) {
    return selModel->selectedRows(column);
  }
  return QModelIndexList();
}

// This returns all selected "cells", which means all cells of the same row are returned.
QModelIndexList FolderView::selectedIndexes() const {
  QItemSelectionModel* selModel = selectionModel();
  if(selModel) {
    return selModel->selectedIndexes();
  }
  return QModelIndexList();
}

QItemSelectionModel* FolderView::selectionModel() const {
  return view ? view->selectionModel() : NULL;
}

FmPathList* FolderView::selectedFilePaths() const {
  if(model_) {
    QModelIndexList selIndexes = mode == DetailedListMode ? selectedRows() : selectedIndexes();
    if(!selIndexes.isEmpty()) {
      FmPathList* paths = fm_path_list_new();
      QModelIndexList::const_iterator it;
      for(it = selIndexes.begin(); it != selIndexes.end(); ++it) {
        FmFileInfo* file = model_->fileInfoFromIndex(*it);
        fm_path_list_push_tail(paths, fm_file_info_get_path(file));
      }
      return paths;
    }
  }
  return NULL;
}

FmFileInfoList* FolderView::selectedFiles() const {
  if(model_) {
    QModelIndexList selIndexes = mode == DetailedListMode ? selectedRows() : selectedIndexes();
    if(!selIndexes.isEmpty()) {
      FmFileInfoList* files = fm_file_info_list_new();
      QModelIndexList::const_iterator it;
      for(it = selIndexes.constBegin(); it != selIndexes.constEnd(); ++it) {
        FmFileInfo* file = model_->fileInfoFromIndex(*it);
        fm_file_info_list_push_tail(files, file);
      }
      return files;
    }
  }
  return NULL;
}

void FolderView::selectAll() {
  if(mode == DetailedListMode)
    view->selectAll();
  else {
    // NOTE: By default QListView::selectAll() selects all columns in the model.
    // However, QListView only show the first column. Normal selection by mouse
    // can only select the first column of every row. I consider this discripancy yet
    // another design flaw of Qt. To make them consistent, we do it ourselves by only
    // selecting the first column of every row and do not select all columns as Qt does.
    // This will trigger one selectionChanged event per row, which is very inefficient,
    // but we have no other choices to workaround the Qt bug.
    // I'll report a Qt bug for this later.
    if(model_) {
      int rowCount = model_->rowCount();
      for(int row = 0; row < rowCount; ++row) {
        QModelIndex index = model_->index(row, 0);
        selectionModel()->select(index, QItemSelectionModel::Select);
      }
    }
  }
}

void FolderView::invertSelection() {
  if(model_) {
    QItemSelectionModel* selModel = view->selectionModel();
    int rows = model_->rowCount();
    QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Toggle;
    if(mode == DetailedListMode)
      flags |= QItemSelectionModel::Rows;
    for(int row = 0; row < rows; ++row) {
      QModelIndex index = model_->index(row, 0);
      selModel->select(index, flags);
    }
  }
}

void FolderView::childDragEnterEvent(QDragEnterEvent* event) {
  qDebug("drag enter");
  if(event->mimeData()->hasFormat("text/uri-list")) {
    event->accept();
  }
  else
    event->ignore();
}

void FolderView::childDragLeaveEvent(QDragLeaveEvent* e) {
  qDebug("drag leave");
  e->accept();
}

void FolderView::childDragMoveEvent(QDragMoveEvent* e) {
  qDebug("drag move");
}

void FolderView::childDropEvent(QDropEvent* e) {
  qDebug("drop");
  if(e->keyboardModifiers() == Qt::NoModifier) {
    // if no key modifiers are used, popup a menu
    // to ask the user for the action he/she wants to perform.
    Qt::DropAction action = DndActionMenu::askUser(QCursor::pos());
    e->setDropAction(action);
  }
}

bool FolderView::eventFilter(QObject* watched, QEvent* event) {
  // NOTE: Instead of simply filtering the drag and drop events of the child view in
  // the event filter, we overrided each event handler virtual methods in
  // both QListView and QTreeView and added some childXXXEvent() callbacks.
  // We did this because of a design flaw of Qt.
  // All QAbstractScrollArea derived widgets, including QAbstractItemView
  // contains an internal child widget, which is called a viewport.
  // The events actually comes from the child viewport, not the parent view itself.
  // Qt redirects the events of viewport to the viewportEvent() method of
  // QAbstractScrollArea and let the parent widget handle the events.
  // Qt implemented this using a event filter installed on the child viewport widget.
  // That means, when we try to install an event filter on the viewport,
  // there is already a filter installed by Qt which will be called before ours.
  // So we can never intercept the event handling of QAbstractItemView by using a filter.
  // That's why we override respective virtual methods for different events.
  if(view && watched == view->viewport()) {
    switch(event->type()) {
    case QEvent::HoverMove:
      // activate items on single click
      if(style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick)) {
        QHoverEvent* hoverEvent = static_cast<QHoverEvent*>(event);
        QModelIndex index = view->indexAt(hoverEvent->pos()); // find out the hovered item
        if(index.isValid()) { // change the cursor to a hand when hovering on an item
          setCursor(Qt::PointingHandCursor);
          if(!selectionModel()->hasSelection())
            selectionModel()->setCurrentIndex(index, QItemSelectionModel::Current);
        }
        else
          setCursor(Qt::ArrowCursor);
        // turn on auto-selection for hovered item when single click mode is used.
        if(autoSelectionDelay_ > 0 && model_) {
          if(!autoSelectionTimer_) {
            autoSelectionTimer_ = new QTimer(this);
            connect(autoSelectionTimer_, &QTimer::timeout, this, &FolderView::onAutoSelectionTimeout);
            lastAutoSelectionIndex_ = QModelIndex();
          }
          autoSelectionTimer_->start(autoSelectionDelay_);
        }
        break;
      }
    case QEvent::HoverLeave:
      if(style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick))
        setCursor(Qt::ArrowCursor);
      break;
    case QEvent::Wheel:
      // This is to fix #85: Scrolling doesn't work in compact view
      // Actually, I think it's the bug of Qt, not ours.
      // When in compact mode, only the horizontal scroll bar is used and the vertical one is hidden.
      // So, when a user scroll his mouse wheel, it's reasonable to scroll the horizontal scollbar.
      // Qt does not implement such a simple feature, unfortunately.
      // We do it by forwarding the scroll event in the viewport to the horizontal scrollbar.
      // FIXME: if someday Qt supports this, we have to disable the workaround.
      if(mode == CompactMode) {
        QScrollBar* scroll = view->horizontalScrollBar();
        if(scroll) {
          QApplication::sendEvent(scroll, event);
          return true;
        }
      }
      break;
    }
  }
  return QObject::eventFilter(watched, event);
}

// this slot handles auto-selection of items.
void FolderView::onAutoSelectionTimeout() {
  if(QApplication::mouseButtons() != Qt::NoButton)
    return;

  Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
  QPoint pos = view->viewport()->mapFromGlobal(QCursor::pos()); // convert to viewport coordinates
  QModelIndex index = view->indexAt(pos); // find out the hovered item
  QItemSelectionModel::SelectionFlags flags = (mode == DetailedListMode ? QItemSelectionModel::Rows : QItemSelectionModel::NoUpdate);
  QItemSelectionModel* selModel = view->selectionModel();

  if(mods & Qt::ControlModifier) { // Ctrl key is pressed
    if(selModel->isSelected(index) && index != lastAutoSelectionIndex_) {
      // unselect a previously selected item
      selModel->select(index, flags|QItemSelectionModel::Deselect);
      lastAutoSelectionIndex_ = QModelIndex();
    }
    else {
      // select an unselected item
      selModel->select(index, flags|QItemSelectionModel::Select);
      lastAutoSelectionIndex_ = index;
    }
    selModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate); // move the cursor
  }
  else if(mods & Qt::ShiftModifier) { // Shift key is pressed
    // select all items between current index and the hovered index.
    QModelIndex current = selModel->currentIndex();
    if(selModel->hasSelection() && current.isValid()) {
      selModel->clear(); // clear old selection
      selModel->setCurrentIndex(current, QItemSelectionModel::NoUpdate);
      int begin = current.row();
      int end = index.row();
      if(begin > end)
        qSwap(begin, end);
      for(int row = begin; row <= end; ++row) {
        QModelIndex sel = model_->index(row, 0);
        selModel->select(sel, flags|QItemSelectionModel::Select);
      }
    }
    else { // no items are selected, select the hovered item.
      if(index.isValid()) {
        selModel->select(index, flags|QItemSelectionModel::SelectCurrent);
        selModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
      }
    }
    lastAutoSelectionIndex_ = index;
  }
  else if(mods == Qt::NoModifier) { // no modifier keys are pressed.
    if(index.isValid()) {
      // select the hovered item
      view->clearSelection();
      selModel->select(index, flags|QItemSelectionModel::SelectCurrent);
      selModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    }
    lastAutoSelectionIndex_ = index;
  }

  autoSelectionTimer_->deleteLater();
  autoSelectionTimer_ = NULL;
}

void FolderView::onFileClicked(int type, FmFileInfo* fileInfo) {
  if(type == ActivatedClick) {
    if(fileLauncher_) {
      GList* files = g_list_append(NULL, fileInfo);
      fileLauncher_->launchFiles(NULL, files);
      g_list_free(files);
    }
  }
  else if(type == ContextMenuClick) {
    FmPath* folderPath = NULL;
    FmFileInfoList* files = selectedFiles();
    if (files) {
      FmFileInfo* first = fm_file_info_list_peek_head(files);
      if (fm_file_info_list_get_length(files) == 1 && fm_file_info_is_dir(first))
        folderPath = fm_file_info_get_path(first);
    }
    if (!folderPath)
      folderPath = path();
    QMenu* menu = NULL;
    if(fileInfo) {
      // show context menu
      if (FmFileInfoList* files = selectedFiles()) {
        Fm::FileMenu* fileMenu = new Fm::FileMenu(files, fileInfo, folderPath);
        fileMenu->setFileLauncher(fileLauncher_);
        prepareFileMenu(fileMenu);
        fm_file_info_list_unref(files);
        menu = fileMenu;
      }
    }
    else {
      Fm::FolderMenu* folderMenu = new Fm::FolderMenu(this);
      prepareFolderMenu(folderMenu);
      menu = folderMenu;
    }
    if (menu) {
      menu->exec(QCursor::pos());
      delete menu;
    }
  }
}

void FolderView::prepareFileMenu(FileMenu* menu) {
}

void FolderView::prepareFolderMenu(FolderMenu* menu) {
}


} // namespace Fm
