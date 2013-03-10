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


#ifndef FM_FILEMENU_H
#define FM_FILEMENU_H

#include <QtGui/QMenu>
#include <libfm/fm.h>

class QAction;

namespace Fm {

class LIBFM_QT_API FileMenu : public QMenu {
Q_OBJECT

public:
  explicit FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, QWidget* parent = 0);
  explicit FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, const QString& title, QWidget* parent = 0);
  ~FileMenu();

  QAction* openAction() {
    return openAction_;  
  }

  QAction* openWithAction() {
    return openWithAction_;  
  }

  QAction* separator1() {
    return separator1_;  
  }

  QAction* cutAction() {
    return cutAction_;  
  }

  QAction* copyAction() {
    return copyAction_;  
  }

  QAction* pasteAction() {
    return pasteAction_;
  }
  
  QAction* deleteAction() {
    return deleteAction_;  
  }

  QAction* renameAction() {
    return renameAction_;  
  }

  QAction* separator2() {
    return separator2_;  
  }

  QAction* propertiesAction() {
    return propertiesAction_;  
  }


protected:
  void createMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd);

protected Q_SLOTS:
  void onOpenTriggered();
  void onFilePropertiesTriggered();
  void onApplicationTriggered();

  void onCutTriggered();
  void onCopyTriggered();
  void onPasteTriggered();
  void onRenameTriggered();
  void onDeleteTriggered();

private:
  FmFileInfoList* files_;
  FmFileInfo* info_;
  FmPath* cwd_;
  bool sameType_;
  bool sameFilesystem_;
  bool allVirtual_;
  bool allTrash_;

  QAction* openAction_;
  QAction* openWithAction_;
  QAction* separator1_;
  QAction* cutAction_;
  QAction* copyAction_;
  QAction* pasteAction_;
  QAction* deleteAction_;
  QAction* renameAction_;
  QAction* separator2_;
  QAction* propertiesAction_;
};

}

#endif // FM_FILEMENU_H
