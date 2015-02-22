/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

#ifndef FM_APPCHOOSERCOMBOBOX_H
#define FM_APPCHOOSERCOMBOBOX_H

#include "libfmqtglobals.h"
#include <QComboBox>
#include <libfm/fm.h>

namespace Fm {

class LIBFM_QT_API AppChooserComboBox : public QComboBox {
  Q_OBJECT
public:
  ~AppChooserComboBox();
  AppChooserComboBox(QWidget* parent);

  void setMimeType(FmMimeType* mimeType);

  FmMimeType* mimeType() {
    return mimeType_;
  }

  GAppInfo* selectedApp();
  // const GList* customApps();

  bool isChanged();

private Q_SLOTS:
  void onCurrentIndexChanged(int index);
  
private:
  FmMimeType* mimeType_;
  GList* appInfos_; // applications used to open the file type
  GAppInfo* defaultApp_; // default application used to open the file type 
  int defaultAppIndex_;
  int prevIndex_;
  bool blockOnCurrentIndexChanged_;
};

}

#endif // FM_APPCHOOSERCOMBOBOX_H
