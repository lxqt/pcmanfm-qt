/*
 * Copyright 2010-2014 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * Copyright 2012-2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifndef FM_APPCHOOSERDIALOG_H
#define FM_APPCHOOSERDIALOG_H

#include <QDialog>
#include "libfmqtglobals.h"
#include <libfm/fm.h>

namespace Ui {
  class AppChooserDialog;
}

namespace Fm {

class LIBFM_QT_API AppChooserDialog : public QDialog {
  Q_OBJECT
public:
  explicit AppChooserDialog(FmMimeType* mimeType, QWidget* parent = NULL, Qt::WindowFlags f = 0);
  ~AppChooserDialog();

  virtual void accept();

  void setMimeType(FmMimeType* mimeType);
  FmMimeType* mimeType() {
    return mimeType_;
  }

  void setCanSetDefault(bool value);
  bool canSetDefault() {
    return canSetDefault_;
  }

  GAppInfo* selectedApp() {
    return G_APP_INFO(g_object_ref(selectedApp_));
  }

  bool isSetDefault();

private:
  GAppInfo* customCommandToApp();

private Q_SLOTS:
  void onSelectionChanged();
  void onTabChanged(int index);

private:
  Ui::AppChooserDialog* ui;
  FmMimeType* mimeType_;
  bool canSetDefault_;
  GAppInfo* selectedApp_;
};

}

#endif // FM_APPCHOOSERDIALOG_H
