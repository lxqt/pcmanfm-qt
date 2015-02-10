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

#include "filesearchdialog.h"
#include <QMessageBox>
#include "fm-search.h"
#include "ui_filesearch.h"
#include <limits>

namespace Fm {

FileSearchDialog::FileSearchDialog(QStringList paths, QWidget* parent, Qt::WindowFlags f):
  QDialog(parent, f),
  ui(new Ui::SearchDialog()) {
  ui->setupUi(this);
  ui->minSize->setMaximum(std::numeric_limits<int>().max());
  ui->maxSize->setMaximum(std::numeric_limits<int>().max());
  Q_FOREACH(const QString& path, paths) {
    ui->listView->addItem(path);
  }
}

FileSearchDialog::~FileSearchDialog() {
  delete ui;
}

void FileSearchDialog::accept() {
  // build the search:/// uri
  int n = ui->listView->count();
  if(n > 0) {
    FmSearch* search = fm_search_new();
    int i;
    for(i = 0; i < n; ++i) { // add directories
      QListWidgetItem* item = ui->listView->item(i);
      fm_search_add_dir(search, item->text().toLocal8Bit().constData());
    }

    fm_search_set_recursive(search, ui->recursiveSearch->isChecked());
    fm_search_set_show_hidden(search, ui->searchHidden->isChecked());
    fm_search_set_name_patterns(search, ui->namePatterns->text().toUtf8().constData());
    fm_search_set_name_ci(search, ui->nameCaseInsensitive->isChecked());
    fm_search_set_name_regex(search, ui->nameRegExp->isChecked());

    fm_search_set_content_pattern(search, ui->contentPattern->text().toUtf8().constData());
    fm_search_set_content_ci(search, ui->contentCaseInsensitive->isChecked());
    fm_search_set_content_regex(search, ui->contentRegExp->isChecked());

    // search for the files of specific mime-types
    if(ui->searchTextFiles->isChecked())
      fm_search_add_mime_type(search, "text/plain");
    if(ui->searchImages->isChecked())
      fm_search_add_mime_type(search, "image/*");
    if(ui->searchAudio->isChecked())
      fm_search_add_mime_type(search, "audio/*");
    if(ui->searchVideo->isChecked())
      fm_search_add_mime_type(search, "video/*");
    //if(ui->search->isChecked())
    //  fm_search_add_mime_type(search, "inode/directory");
    if(ui->searchDocuments->isChecked()) {
      const char* doc_types[] = {
        "application/pdf;"
        /* "text/html;" */
        "application/vnd.oasis.opendocument.*;"
        "application/vnd.openxmlformats-officedocument.*;"
        "application/msword;application/vnd.ms-word;"
        "application/msexcel;application/vnd.ms-excel"
      };
      for(i = 0; i < sizeof(doc_types)/sizeof(char*); ++i)
	fm_search_add_mime_type(search, doc_types[i]);
    }
    
    // search based on file size
    const unsigned int unit_bytes[] = {1, (1024), (1024*1024), (1024*1024*1024)};
    if(ui->largerThan->isChecked()) {
      guint64 size = ui->minSize->value() * unit_bytes[ui->minSizeUnit->currentIndex()];
      fm_search_set_min_size(search, size);
    }

    if(ui->smallerThan->isChecked()) {
      guint64 size = ui->maxSize->value() * unit_bytes[ui->maxSizeUnit->currentIndex()];
      fm_search_set_min_size(search, size);
    }

    // search based on file mtime (we only support date in YYYY-MM-DD format)
    if(ui->earlierThan->isChecked()) {
      fm_search_set_max_mtime(search, ui->maxTime->date().toString(QStringLiteral("yyyy-MM-dd")).toUtf8().constData());
    }
    if(ui->laterThan->isChecked()) {
      fm_search_set_min_mtime(search, ui->minTime->date().toString(QStringLiteral("yyyy-MM-dd")).toUtf8().constData());
    }

    searchUri_.take(fm_search_dup_path(search));

    fm_search_free(search);
  }
  else {
    QMessageBox::critical(this, tr("Error"), tr("You should add at least add one directory to search."));
    return;
  }
  QDialog::accept();
}

}
