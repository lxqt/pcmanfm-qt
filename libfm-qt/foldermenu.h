/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  PCMan <email>

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


#ifndef FM_FOLDERMENU_H
#define FM_FOLDERMENU_H

#include <QMenu>
#include <libfm/fm.h>

class QAction;

namespace Fm {

class FolderView;
  
class FolderMenu : public QMenu {
Q_OBJECT

public:
  explicit FolderMenu(FolderView* view, QWidget* parent = 0);
  virtual ~FolderMenu();

  QAction* createAction() {
    return createAction_;  
  }

  QAction* separator1() {
    return separator1_;  
  }

  QAction* pasteAction() {
    return pasteAction_;  
  }

  QAction* separator2() {
    return separator2_;  
  }

  QAction* selectAllAction() {
    return selectAllAction_;  
  }

  QAction* invertSelectionAction() {
    return invertSelectionAction_;  
  }

  QAction* separator3() {
    return separator3_;  
  }

  QAction* sortAction() {
    return sortAction_;  
  }

  QAction* showHiddenAction() {
    return showHiddenAction_;  
  }

  QAction* separator4() {
    return separator4_;  
  }

  QAction* propertiesAction() {
    return propertiesAction_;  
  }

protected Q_SLOTS:

  void onPasteActionTriggered();

  void onSelectAllActionTriggered();

  void onInvertSelectionActionTriggered();

  void onSortActionTriggered();

  void onShowHiddenActionTriggered(bool checked);

  void onPropertiesActionTriggered();

private:
  QMenu* createCreateNewMenu();
  QMenu* createSortMenu();

private:
  FolderView* view_;
  QAction* createAction_;
  QAction* separator1_;
  QAction* pasteAction_;
  QAction* separator2_;
  QAction* selectAllAction_;
  QAction* invertSelectionAction_;
  QAction* separator3_;
  QAction* sortAction_;
  QAction* showHiddenAction_;
  QAction* separator4_;
  QAction* propertiesAction_;
};

}

#endif // FM_FOLDERMENU_H
