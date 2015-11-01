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
#include "foldermodel.h"
#include <QPainter>
#include <QModelIndex>
#include <QStyleOptionViewItemV4>
#include <QApplication>
#include <QIcon>
#include <QTextLayout>
#include <QTextOption>
#include <QTextLine>
#include <QDebug>

namespace Fm {

FolderItemDelegate::FolderItemDelegate(QAbstractItemView* view, QObject* parent):
  QStyledItemDelegate(parent ? parent : view),
  symlinkIcon_(QIcon::fromTheme("emblem-symbolic-link")),
  view_(view) {
}

FolderItemDelegate::~FolderItemDelegate() {

}

QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  QVariant value = index.data(Qt::SizeHintRole);
  if(value.isValid())
    return qvariant_cast<QSize>(value);
  if(option.decorationPosition == QStyleOptionViewItem::Top ||
    option.decorationPosition == QStyleOptionViewItem::Bottom) {

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    opt.decorationAlignment = Qt::AlignHCenter|Qt::AlignTop;
    opt.displayAlignment = Qt::AlignTop|Qt::AlignHCenter;

    // FIXME: there're some problems in this size hint calculation.
    Q_ASSERT(gridSize_ != QSize());
    QRectF textRect(0, 0, gridSize_.width() - 4, gridSize_.height() - opt.decorationSize.height() - 4);
    drawText(nullptr, opt, textRect); // passing nullptr for painter will calculate the bounding rect only.
    int width = qMax((int)textRect.width(), opt.decorationSize.width()) + 4;
    int height = opt.decorationSize.height() + textRect.height() + 4;
    return QSize(width, height);
  }
  return QStyledItemDelegate::sizeHint(option, index);
}

QIcon::Mode FolderItemDelegate::iconModeFromState(const QStyle::State state) {

  if(state & QStyle::State_Enabled)
    return (state & QStyle::State_Selected) ? QIcon::Selected : QIcon::Normal;

  return QIcon::Disabled;
}

// special thanks to Razor-qt developer Alec Moskvin(amoskvin) for providing the fix!
void FolderItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  Q_ASSERT(index.isValid());
  FmFileInfo* file = static_cast<FmFileInfo*>(index.data(FolderModel::FileInfoRole).value<void*>());
  bool isSymlink = file && fm_file_info_is_symlink(file);

  if(option.decorationPosition == QStyleOptionViewItem::Top ||
    option.decorationPosition == QStyleOptionViewItem::Bottom) {
    painter->save();
    painter->setClipRect(option.rect);

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    opt.decorationAlignment = Qt::AlignHCenter|Qt::AlignTop;
    opt.displayAlignment = Qt::AlignTop|Qt::AlignHCenter;

    // draw the icon
    QIcon::Mode iconMode = iconModeFromState(opt.state);
    QPoint iconPos(opt.rect.x() + (opt.rect.width() - opt.decorationSize.width()) / 2, opt.rect.y());
    QPixmap pixmap = opt.icon.pixmap(opt.decorationSize, iconMode);
    painter->drawPixmap(iconPos, pixmap);

    // draw some emblems for the item if needed
    // we only support symlink emblem at the moment
    if(isSymlink)
      painter->drawPixmap(iconPos, symlinkIcon_.pixmap(opt.decorationSize / 2, iconMode));

    // draw the text
    QRectF textRect(opt.rect.x(), opt.rect.y() + opt.decorationSize.height(), opt.rect.width(), opt.rect.height() - opt.decorationSize.height());
    drawText(painter, opt, textRect);
    painter->restore();
  }
  else {
    // let QStyledItemDelegate does its default painting
    QStyledItemDelegate::paint(painter, option, index);

    // draw emblems if needed
    if(isSymlink) {
      QStyleOptionViewItemV4 opt = option;
      initStyleOption(&opt, index);
      QIcon::Mode iconMode = iconModeFromState(opt.state);
      QPoint iconPos(opt.rect.x(), opt.rect.y() + (opt.rect.height() - opt.decorationSize.height()) / 2);
      // draw some emblems for the item if needed
      // we only support symlink emblem at the moment
      painter->drawPixmap(iconPos, symlinkIcon_.pixmap(opt.decorationSize / 2, iconMode));
    }
  }
}

// if painter is nullptr, the method calculate the bounding rectangle of the text and save it to textRect
void FolderItemDelegate::drawText(QPainter* painter, QStyleOptionViewItemV4& opt, QRectF& textRect) const {
  QTextLayout layout(opt.text, opt.font);
  QTextOption textOption;
  textOption.setAlignment(opt.displayAlignment);
  textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  textOption.setTextDirection(opt.direction);
  layout.setTextOption(textOption);
  qreal height = 0;
  qreal width = 0;
  int visibleLines = 0;
  layout.beginLayout();
  QString elidedText;
  for(;;) {
    QTextLine line = layout.createLine();
    if(!line.isValid())
      break;
    line.setLineWidth(textRect.width());
    height += opt.fontMetrics.leading();
    line.setPosition(QPointF(0, height));
    if((height + line.height() + textRect.y()) > textRect.bottom()) {
      // if part of this line falls outside the textRect, ignore it and quit.
      QTextLine lastLine = layout.lineAt(visibleLines - 1);
      elidedText = opt.text.mid(lastLine.textStart());
      elidedText = opt.fontMetrics.elidedText(elidedText, opt.textElideMode, textRect.width());
      if(visibleLines == 1) // this is the only visible line
        width = textRect.width();
      break;
    }
    height += line.height();
    width = qMax(width, line.naturalTextWidth());
    ++ visibleLines;
  }
  layout.endLayout();

  // draw background for selected item
  QRectF boundRect = layout.boundingRect();
  //qDebug() << "bound rect: " << boundRect << "width: " << width;
  boundRect.setWidth(width);
  boundRect.moveTo(textRect.x() + (textRect.width() - width)/2, textRect.y());

  if(!painter) { // no painter, calculate the bounding rect only
    textRect = boundRect;
    return;
  }

  QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
  if(opt.state & QStyle::State_Selected) {
    painter->fillRect(boundRect, opt.palette.highlight());
    painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
  }
  else
    painter->setPen(opt.palette.color(cg, QPalette::Text));

  // draw text
  for(int i = 0; i < visibleLines; ++i) {
    QTextLine line = layout.lineAt(i);
    if(i == (visibleLines - 1) && !elidedText.isEmpty()) { // the last line, draw elided text
      QPointF pos(textRect.x() + line.position().x(), textRect.y() + line.y() + line.ascent());
      painter->drawText(pos, elidedText);
    }
    else {
      line.draw(painter, textRect.topLeft());
    }
  }

  if(opt.state & QStyle::State_HasFocus) {
    // draw focus rect
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=(opt);
    o.rect = boundRect.toRect(); // subElementRect(SE_ItemViewItemFocusRect, vopt, widget);
    o.state |= QStyle::State_KeyboardFocusChange;
    o.state |= QStyle::State_Item;
    QPalette::ColorGroup cg = (opt.state & QStyle::State_Enabled)
                  ? QPalette::Normal : QPalette::Disabled;
    o.backgroundColor = opt.palette.color(cg, (opt.state & QStyle::State_Selected)
                                  ? QPalette::Highlight : QPalette::Window);
    if (const QWidget* widget = opt.widget) {
      QStyle* style = widget->style() ? widget->style() : qApp->style();
      style->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter, widget);
    }
  }
}


} // namespace Fm
