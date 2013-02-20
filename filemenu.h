/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

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

namespace Fm {

class FileMenu : public QMenu
{
Q_OBJECT

public:
  explicit FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, QWidget* parent = 0);
  explicit FileMenu(FmFileInfoList* files, FmFileInfo* info, FmPath* cwd, const QString& title, QWidget* parent = 0);
  ~FileMenu();
  
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
};

}

#endif // FM_FILEMENU_H
