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

#include "libfmqtglobals.h"
#include <QMenu>
#include <qabstractitemmodel.h>
#include <libfm/fm.h>

class QAction;

struct _FmFileActionItem;

namespace Fm {

class FileLauncher;

class LIBFM_QT_API FileMenu : public QMenu {
Q_OBJECT

public:
  explicit FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, QWidget* parent = 0);
  explicit FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, const QString& title, QWidget* parent = 0);
  ~FileMenu();

  bool useTrash() {
    return useTrash_;
  }

  void setUseTrash(bool trash);

  bool confirmDelete() {
    return confirmDelete_;
  }

  void setConfirmDelete(bool confirm) {
    confirmDelete_ = confirm;
  }

  QAction* openAction() {
    return openAction_;
  }

  QAction* openWithMenuAction() {
    return openWithMenuAction_;
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

  QAction* unTrashAction() {
    return unTrashAction_;
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

  FmFileInfoList* files() {
    return files_;
  }

  FmFileInfo* firstFile() {
    return info_;
  }

  FmPath* cwd() {
    return cwd_;
  }

  void setFileLauncher(FileLauncher* launcher) {
    fileLauncher_ = launcher;
  }

  FileLauncher* fileLauncher() {
    return fileLauncher_;
  }

  bool sameType() const {
    return sameType_;
  }

  bool sameFilesystem() const {
    return sameFilesystem_;
  }

  bool allVirtual() const {
    return allVirtual_;
  }

  bool allTrash() const {
    return allTrash_;
  }

protected:
  void createMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd);
#ifdef CUSTOM_ACTIONS
  void addCustomActionItem(QMenu* menu, struct _FmFileActionItem* item);
#endif
  void openFilesWithApp(GAppInfo* app);

protected Q_SLOTS:
  void onOpenTriggered();
  void onOpenWithTriggered();
  void onFilePropertiesTriggered();
  void onApplicationTriggered();
#ifdef CUSTOM_ACTIONS
  void onCustomActionTrigerred();
#endif
  void onCompress();
  void onExtract();
  void onExtractHere();

  void onCutTriggered();
  void onCopyTriggered();
  void onPasteTriggered();
  void onRenameTriggered();
  void onDeleteTriggered();
  void onUnTrashTriggered();

private:
  FmFileInfoList* files_;
  FmFileInfo* info_;
  FmPath* cwd_;
  bool useTrash_;
  bool confirmDelete_;
  bool sameType_;
  bool sameFilesystem_;
  bool allVirtual_;
  bool allTrash_;

  QAction* openAction_;
  QAction* openWithMenuAction_;
  QAction* openWithAction_;
  QAction* separator1_;
  QAction* cutAction_;
  QAction* copyAction_;
  QAction* pasteAction_;
  QAction* deleteAction_;
  QAction* unTrashAction_;
  QAction* renameAction_;
  QAction* separator2_;
  QAction* propertiesAction_;

  FileLauncher* fileLauncher_;
};

}

#endif // FM_FILEMENU_H
