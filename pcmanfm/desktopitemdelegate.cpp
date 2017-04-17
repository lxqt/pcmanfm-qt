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
#include <libfm-qt/foldermodel.h>
#include <libfm-qt/core/fileinfo.h>
#include <QApplication>
#include <QListView>
#include <QPainter>
#include <QIcon>
#include <QTextLayout>
#include <QTextOption>
#include <QTextLine>

namespace PCManFM {

DesktopItemDelegate::DesktopItemDelegate(QListView* view, QObject* parent):
    QStyledItemDelegate(parent ? parent : view),
    view_(view),
    symlinkIcon_(QIcon::fromTheme("emblem-symbolic-link")),
    shadowColor_(0, 0, 0),
    margins_(QSize(3, 3)) {
}

// FIXME: we need to figure out a way to derive from Fm::FolderItemDelegate to avoid code duplication.
void DesktopItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_ASSERT(index.isValid());
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->save();
    painter->setClipRect(option.rect);

    opt.decorationAlignment = Qt::AlignHCenter | Qt::AlignTop;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    // draw the icon
    QIcon::Mode iconMode;
    if(opt.state & QStyle::State_Enabled) {
        if(opt.state & QStyle::State_Selected) {
            iconMode = QIcon::Selected;
        }
        else {
            iconMode = QIcon::Normal;
        }
    }
    else {
        iconMode = QIcon::Disabled;
    }
    QPoint iconPos(opt.rect.x() + (opt.rect.width() - option.decorationSize.width()) / 2, opt.rect.y());
    QPixmap pixmap = opt.icon.pixmap(option.decorationSize, iconMode);
    // in case the pixmap is smaller than the requested size
    QSize margin = ((option.decorationSize - pixmap.size()) / 2).expandedTo(QSize(0, 0));
    painter->drawPixmap(iconPos + QPoint(margin.width(), margin.height()), pixmap);

    // draw some emblems for the item if needed
    // we only support symlink emblem at the moment
    auto file = index.data(Fm::FolderModel::FileInfoRole).value<std::shared_ptr<const Fm::FileInfo>>();
    if(file) {
        if(file->isSymlink()) {
            painter->drawPixmap(iconPos, symlinkIcon_.pixmap(option.decorationSize / 2, iconMode));
        }
    }
    // FIXME: draw other emblems

    // draw text
    QSize gridSize = view_->gridSize() - 2 * margins_;
    QRectF textRect(opt.rect.x() - (gridSize.width() - opt.rect.width()) / 2,
                    opt.rect.y() + option.decorationSize.height(),
                    gridSize.width(),
                    gridSize.height() - option.decorationSize.height());
    drawText(painter, opt, textRect);

    if(opt.state & QStyle::State_HasFocus) {
        // FIXME: draw focus rect
    }
    painter->restore();
}

void DesktopItemDelegate::drawText(QPainter* painter, QStyleOptionViewItem& opt, QRectF& textRect) const {
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
    textRect.adjust(2, 2, -2, -2); // a 2-px margin is considered at FolderView::updateGridSize()
    for(;;) {
        QTextLine line = layout.createLine();
        if(!line.isValid()) {
            break;
        }
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
    width = qMax(width, (qreal)opt.fontMetrics.width(elidedText));
    QRectF boundRect = layout.boundingRect();
    boundRect.setWidth(width);
    boundRect.setHeight(height);
    boundRect.moveTo(textRect.x() + (textRect.width() - width) / 2, textRect.y());

    QRectF selRect = boundRect.adjusted(-2, -2, 2, 2);

    if(!painter) { // no painter, calculate the bounding rect only
        textRect = selRect;
        return;
    }

    if(opt.state & QStyle::State_Selected || opt.state & QStyle::State_MouseOver) {
        if(const QWidget* widget = opt.widget) {  // let the style engine do it
            QStyle* style = widget->style() ? widget->style() : qApp->style();
            QStyleOptionViewItem o(opt);
            o.text = QString();
            o.rect = selRect.toAlignedRect().intersected(opt.rect); // due to clipping and rounding, we might lose 1px
            o.showDecorationSelected = true;
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, &o, painter, widget);
        }
    }

    if((opt.state & QStyle::State_Selected)) {
        // qDebug("w: %f, h:%f, m:%f", boundRect.width(), boundRect.height(), layout.minimumWidth());
        if(!opt.widget) {
            painter->fillRect(selRect, opt.palette.highlight());
        }
        painter->setPen(opt.palette.color(QPalette::Normal, QPalette::HighlightedText));
    }
    else { // only draw shadow for non-selected items
        // draw shadow, FIXME: is it possible to use QGraphicsDropShadowEffect here?
        QPen prevPen = painter->pen();
        painter->setPen(QPen(shadowColor_));
        for(int i = 0; i < visibleLines; ++i) {
            QTextLine line = layout.lineAt(i);
            if(i == (visibleLines - 1) && !elidedText.isEmpty()) { // the last line, draw elided text
                QPointF pos(boundRect.x() + line.position().x() + 1, boundRect.y() + line.y() + line.ascent() + 1);
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
            QPointF pos(boundRect.x() + line.position().x(), boundRect.y() + line.y() + line.ascent());
            painter->drawText(pos, elidedText);
        }
        else {
            line.draw(painter, textRect.topLeft());
        }
    }
}

QSize DesktopItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QVariant value = index.data(Qt::SizeHintRole);
    if(value.isValid()) {
        return qvariant_cast<QSize>(value);
    }
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.decorationAlignment = Qt::AlignHCenter | Qt::AlignTop;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    QSize gridSize = view_->gridSize() - 2 * margins_;
    Q_ASSERT(gridSize != QSize());
    QRectF textRect(0, 0, gridSize.width(), gridSize.height() - option.decorationSize.height());
    drawText(nullptr, opt, textRect); // passing nullptr for painter will calculate the bounding rect only.
    int width = qMax((int)textRect.width(), option.decorationSize.width());
    int height = option.decorationSize.height() + textRect.height();
    return QSize(width, height);
}

DesktopItemDelegate::~DesktopItemDelegate() {

}

} // namespace PCManFM
