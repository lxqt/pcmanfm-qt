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


#include "folderitemdelegate.h"
#include <QPainter>
#include <QModelIndex>
#include <QStyleOptionViewItemV4>
#include <QApplication>
#include <QIcon>

using namespace Fm;

FolderItemDelegate::FolderItemDelegate(QListView* view, QObject* parent):
  QStyledItemDelegate(parent ? parent : view),
  view_(view) {

}

FolderItemDelegate::~FolderItemDelegate() {

}

// special thanks to Razor-qt developer Alec Moskvin(amoskvin) for providing the fix!
QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  QVariant value = index.data(Qt::SizeHintRole);
  if(value.isValid())
    return qvariant_cast<QSize>(value);
  if(option.decorationPosition == QStyleOptionViewItem::Top ||
    option.decorationPosition == QStyleOptionViewItem::Bottom) {

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    const QWidget* widget = opt.widget;
    QStyle* style = widget ? widget->style() : QApplication::style();

    QSize gridSize = view_->gridSize();
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    if(size.height() > gridSize.height())
      size.setHeight(gridSize.height());
    // make the item use the width of the whole grid, not the width of the icon.
    size.setWidth(gridSize.width());
    return size;
  }
  return QStyledItemDelegate::sizeHint(option, index);
}

// special thanks to Razor-qt developer Alec Moskvin(amoskvin) for providing the fix!
void FolderItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  Q_ASSERT(index.isValid());
  if(option.decorationPosition == QStyleOptionViewItem::Top ||
    option.decorationPosition == QStyleOptionViewItem::Bottom) {
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    opt.decorationAlignment = Qt::AlignHCenter|Qt::AlignTop;
    opt.displayAlignment = Qt::AlignTop|Qt::AlignHCenter;

    // FIXME: duplicate the pixmap here seems to cause some high memory usage
    // problems, and the pixmap previously cached for the QIcon cannot be used, too.
    // We need to implement our own full item delegate to pain the item properly and
    // remove this hack as soon as possible.
    QPixmap pixmap = opt.icon.pixmap(view_->iconSize());
    opt.decorationSize.setWidth(view_->gridSize().width());
    opt.decorationSize.setHeight(view_->iconSize().height());
    opt.icon = QIcon(pixmap.copy(QRect(QPoint(0, 0), opt.decorationSize)));
    view_->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter);
  }
  else {
    QStyledItemDelegate::paint(painter, option, index);
  }
}
