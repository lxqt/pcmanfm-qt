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


#include "editbookmarksdialog.h"
#include "ui_edit-bookmarks.h"
#include <QByteArray>
#include <QUrl>

using namespace Fm;

EditBookmarksDialog::EditBookmarksDialog(FmBookmarks* bookmarks, QWidget* parent, Qt::WindowFlags f):
  QDialog(parent, f),
  ui(new Ui::EditBookmarksDialog()),
  bookmarks_(g_object_ref(bookmarks)) {

  ui->setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose); // auto delete on close

  // load bookmarks
  GList* l = fm_bookmarks_get_all(bookmarks_);
  for(; l; l = l->next) {
    FmBookmarkItem* bookmark = reinterpret_cast<FmBookmarkItem*>(l->data);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    char* path_str = fm_path_display_name(bookmark->path, false);
    item->setData(0, Qt::DisplayRole, QString::fromUtf8(bookmark->name));
    item->setData(1, Qt::DisplayRole, QString::fromUtf8(path_str));
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsEnabled);
    g_free(path_str);
    ui->treeWidget->addTopLevelItem(item);
  }

  connect(ui->addItem, SIGNAL(clicked(bool)), SLOT(onAddItem()));
  connect(ui->removeItem, SIGNAL(clicked(bool)), SLOT(onRemoveItem()));
}

EditBookmarksDialog::~EditBookmarksDialog() {
  g_object_unref(bookmarks_);
  delete ui;
}

void EditBookmarksDialog::accept() {
  // save bookmarks
  // it's easier to recreate the whole bookmark file than
  // to manipulate FmBookmarks object. So here we generate the file directly.
  QByteArray buf;
  buf.reserve(4096);
  for(int row = 0; ; ++row) {
    QTreeWidgetItem* item = ui->treeWidget->topLevelItem(row);
    if(!item)
      break;
    QString name = item->data(0, Qt::DisplayRole).toString();
    QString path = item->data(1, Qt::DisplayRole).toString();
    QUrl url = QUrl::fromUserInput(path);
    buf.append(url.toEncoded());
    buf.append(' ');
    buf.append(name.toUtf8());
    buf.append('\n');
  }

  // FIXME: should we support Qt or KDE specific bookmarks in the future?
  char* outputFile = g_build_filename(g_get_home_dir(), ".gtk-bookmarks", NULL);
  // we use glib API here because the API is atomic.
  g_file_set_contents(outputFile, buf.constData(), buf.length(), NULL);
  g_free(outputFile);
  QDialog::accept();
}

void EditBookmarksDialog::onAddItem() {
  QTreeWidgetItem* item = new QTreeWidgetItem();
  item->setData(0, Qt::DisplayRole, tr("New bookmark"));
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsEnabled);
  ui->treeWidget->addTopLevelItem(item);
  ui->treeWidget->editItem(item);
}

void EditBookmarksDialog::onRemoveItem() {
  QList<QTreeWidgetItem*> sels = ui->treeWidget->selectedItems();
  Q_FOREACH(QTreeWidgetItem* item, sels) {
    delete item;
  }
}

#include "editbookmarksdialog.moc"


