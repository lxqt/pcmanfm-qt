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


#ifndef PCMANFM_DESKTOPITEMDELEGATE_H
#define PCMANFM_DESKTOPITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QColor>

class QListView;
class QTextOption;
class QTextLayout;

namespace PCManFM {

class DesktopItemDelegate : public QStyledItemDelegate
{
Q_OBJECT
public:
  explicit DesktopItemDelegate(QListView* view, QObject* parent = 0);
  virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
  virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
  virtual ~DesktopItemDelegate();

  void setShadowColor(const QColor& shadowColor) {
    shadowColor_ = shadowColor;
  }
  const QColor& shadowColor() const {
    return shadowColor_;
  }
  void setMargins(QSize margins) {
    margins_ = margins.expandedTo(QSize(0, 0));
  }

private:
  void drawText(QPainter* painter, QStyleOptionViewItem& opt, QRectF& textRect) const;

private:
  QListView* view_;
  QIcon symlinkIcon_;
  QColor shadowColor_;
  QSize margins_;
};

}

#endif // PCMANFM_DESKTOPITEMDELEGATE_H
