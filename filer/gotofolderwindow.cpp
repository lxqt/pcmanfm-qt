/*

    Copyright (C) 2021  Chris Moore <chris@mooreonline.org>

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

#include "gotofolderwindow.h"

#include <QCompleter>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFileSystemModel>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

using namespace Filer;

GotoFolderDialog::GotoFolderDialog(QWidget *parent)
  : QDialog(parent),
    lineEdit_(0)
{
  auto layout = new QVBoxLayout;

  auto label = new QLabel;
  label->setText("Go to the folder:");
  layout->addWidget(label);

  lineEdit_ = new QLineEdit();
  lineEdit_->setMinimumWidth(480);
  layout->addWidget(lineEdit_);

  auto buttonBox = new QDialogButtonBox;
  buttonBox->addButton(tr("Go"), QDialogButtonBox::AcceptRole);
  buttonBox->addButton(tr("Cancel"),QDialogButtonBox::RejectRole);

  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  // TODO: Implement functionality

  layout->addWidget(buttonBox);

  setLayout(layout);

  setWindowTitle(tr("Go To Folder"));
  lineEdit_->setFocus();

  auto completer = new QCompleter(this);
  completer->setCompletionMode(QCompleter::InlineCompletion);
  QFileSystemModel *fsModel = new QFileSystemModel(completer);
  fsModel->setFilter(QDir::Dirs|QDir::Drives|QDir::NoDotAndDotDot|QDir::AllDirs); // Only directories, no files
  completer->setModel(fsModel);
  fsModel->setRootPath(QString());
  lineEdit_->setCompleter(completer);
}

GotoFolderDialog::~GotoFolderDialog()
{
  // do nothing
}

QString GotoFolderDialog::getPath()
{
  return lineEdit_->text();
}
