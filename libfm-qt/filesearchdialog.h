/*
 * Copyright (C) 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 */

#ifndef FM_FILESEARCHDIALOG_H
#define FM_FILESEARCHDIALOG_H

#include "libfmqtglobals.h"
#include <QDialog>
#include "path.h"

namespace Ui {
  class SearchDialog;
}

namespace Fm {

class LIBFM_QT_API FileSearchDialog : public QDialog
{
public:
  FileSearchDialog(QStringList paths = QStringList(), QWidget * parent = 0, Qt::WindowFlags f = 0);
  ~FileSearchDialog();

  Path searchUri() const {
    return searchUri_;
  }

  virtual void accept();

private:
  Ui::SearchDialog* ui;
  Path searchUri_;
};

}

#endif // FM_FILESEARCHDIALOG_H
