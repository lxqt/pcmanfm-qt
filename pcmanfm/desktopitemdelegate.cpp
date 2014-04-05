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


#include "desktopitemdelegate.h"
#include "foldermodel.h"
#include <QApplication>
#include <QListView>
#include <QPainter>
#include <QIcon>
#include <QTextLayout>
#include <QTextOption>
#include <QTextLine>

using namespace PCManFM;

DesktopItemDelegate::DesktopItemDelegate(QListView* view, QObject* parent):
  QStyledItemDelegate(parent ? parent : view),
  view_(view),
  symlinkIcon_(QIcon::fromTheme("emblem-symbolic-link")),
  shadowColor_(0, 0, 0) {
}

// FIXME: we need to figure out a way to derive from Fm::FolderItemDelegate to avoid code duplication.
void DesktopItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  Q_ASSERT(index.isValid());
  QStyleOptionViewItemV4 opt = option;
  initStyleOption(&opt, index);
  const QWidget* widget = opt.widget;
  QStyle* style = widget ? widget->style() : QApplication::style();

  painter->save();
  painter->setClipRect(option.rect);

  opt.decorationAlignment = Qt::AlignHCenter | Qt::AlignTop;
  opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

  // draw the icon
  QIcon::Mode iconMode;
  if(opt.state & QStyle::State_Enabled) {
    if(opt.state & QStyle::State_Selected)
      iconMode = QIcon::Selected;
    else {
      iconMode = QIcon::Normal;
    }
  }
  else
    iconMode = QIcon::Disabled;
  QPoint iconPos(opt.rect.x() + (opt.rect.width() - opt.decorationSize.width()) / 2, opt.rect.y());
  QPixmap pixmap = opt.icon.pixmap(opt.decorationSize, iconMode);
  painter->drawPixmap(iconPos, pixmap);

  // draw some emblems for the item if needed
  // we only support symlink emblem at the moment
#if QT_VERSION >= 0x050000
  FmFileInfo* file = static_cast<FmFileInfo*>(index.data(Fm::FolderModel::FileInfoRole).value<void*>());
#else
  FmFileInfo* file = static_cast<FmFileInfo*>(qVariantValue<void*>(index.data(Fm::FolderModel::FileInfoRole)));
#endif
  if(file) {
    if(fm_file_info_is_symlink(file)) {
      painter->drawPixmap(iconPos, symlinkIcon_.pixmap(opt.decorationSize / 2, iconMode));
    }
  }
  
  // draw text
  QRectF textRect(opt.rect.x(), opt.rect.y() + opt.decorationSize.height(), opt.rect.width(), opt.rect.height() - opt.decorationSize.height());
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
      break;
    }
    height += line.height();
    width = qMax(width, line.naturalTextWidth());
    ++ visibleLines;
  }
  layout.endLayout();
  QRectF boundRect = layout.boundingRect();
  boundRect.setWidth(width);
  boundRect.moveTo(textRect.x() + (textRect.width() - width)/2, textRect.y());
  if(opt.state & QStyle::State_Selected) {
    QPalette palette = widget->palette();
    // qDebug("w: %f, h:%f, m:%f", boundRect.width(), boundRect.height(), layout.minimumWidth());
    painter->fillRect(boundRect, palette.highlight());
  }
  else { // only draw shadow for non-selected items
    // draw shadow, FIXME: is it possible to use QGraphicsDropShadowEffect here?
    QPen prevPen = painter->pen();
    painter->setPen(QPen(shadowColor_));
    for(int i = 0; i < visibleLines; ++i) {
      QTextLine line = layout.lineAt(i);
      if(i == (visibleLines - 1) && !elidedText.isEmpty()) { // the last line, draw elided text
        QPointF pos(textRect.x() + line.position().x() + 1, textRect.y() + line.y() + line.ascent() + 1);
        painter->drawText(pos, elidedText);
      }
      else {
        line.draw(painter, textRect.topLeft() + QPointF(1, 1));
      }
    }
    painter->setPen(prevPen);
  }

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
    // FIXME: draw focus rect
  }
  painter->restore();
}

QSize DesktopItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  QVariant value = index.data(Qt::SizeHintRole);
  if(value.isValid())
    return qvariant_cast<QSize>(value);
  QStyleOptionViewItemV4 opt = option;
  initStyleOption(&opt, index);
  const QWidget* widget = opt.widget;
  QStyle* style = widget ? widget->style() : QApplication::style();

  // use grid size as size hint
  QSize gridSize = view_->gridSize();
  return QSize(gridSize.width() -2, gridSize.height() - 2);
}

DesktopItemDelegate::~DesktopItemDelegate() {

}
