/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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


#include "utilities.h"
#include "utilities_p.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QList>
#include <QStringBuilder>
#include <QMessageBox>
#include "fileoperation.h"
#include <QEventLoop>

#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <glib.h>

using namespace Fm;

namespace Fm {

FmPathList* pathListFromQUrls(QList<QUrl> urls) {
  QList<QUrl>::const_iterator it;
  FmPathList* pathList = fm_path_list_new();

  for(it = urls.begin(); it != urls.end(); ++it) {
    QUrl url = *it;
    FmPath* path = fm_path_new_for_uri(url.toString().toUtf8());
    fm_path_list_push_tail(pathList, path);
    fm_path_unref(path);
  }

  return pathList;
}

void pasteFilesFromClipboard(FmPath* destPath, QWidget* parent) {
  QClipboard* clipboard = QApplication::clipboard();
  const QMimeData* data = clipboard->mimeData();
  bool isCut = false;
  FmPathList* paths = NULL;

  if(data->hasFormat("x-special/gnome-copied-files")) {
    // Gnome, LXDE, and XFCE
    QByteArray gnomeData = data->data("x-special/gnome-copied-files");
    char* pdata = gnomeData.data();
    char* eol = strchr(pdata, '\n');

    if(eol) {
      *eol = '\0';
      isCut = (strcmp(pdata, "cut") == 0 ? true : false);
      paths = fm_path_list_new_from_uri_list(eol + 1);
    }
  }

  if(!paths && data->hasUrls()) {
    // The KDE way
    paths = Fm::pathListFromQUrls(data->urls());
    QByteArray cut = data->data("x-kde-cut-selection");

    if(!cut.isEmpty() && cut.at(0) == '1')
      isCut = true;
  }

  if(paths) {
    if(isCut)
      FileOperation::moveFiles(paths, destPath, parent);
    else
      FileOperation::copyFiles(paths, destPath, parent);

    fm_path_list_unref(paths);
  }
}

void copyFilesToClipboard(FmPathList* files) {
  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* data = new QMimeData();
  char* urilist = fm_path_list_to_uri_list(files);
  // Gnome, LXDE, and XFCE
  data->setData("x-special/gnome-copied-files", (QString("copy\n") + urilist).toUtf8());
  // The KDE way
  data->setData("text/uri-list", urilist);
  // data.setData("x-kde-cut-selection", "0");
  g_free(urilist);
  clipboard->setMimeData(data);
}

void cutFilesToClipboard(FmPathList* files) {
  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* data = new QMimeData();
  char* urilist = fm_path_list_to_uri_list(files);
  // Gnome, LXDE, and XFCE
  data->setData("x-special/gnome-copied-files", (QString("cut\n") + urilist).toUtf8());
  // The KDE way
  data->setData("text/uri-list", urilist);
  data->setData("x-kde-cut-selection", "1");
  g_free(urilist);
  clipboard->setMimeData(data);
}

void renameFile(FmFileInfo *file, QWidget *parent) {
  FmPath* path = fm_file_info_get_path(file);
  FilenameDialog dlg(parent);
  dlg.setWindowTitle(QObject::tr("Rename File"));
  dlg.setLabelText(QObject::tr("Please enter a new name:"));
  // FIXME: what's the best way to handle non-UTF8 filename encoding here?
  QString old_name = QString::fromLocal8Bit(fm_path_get_basename(path));
  dlg.setTextValue(old_name);

  if(fm_file_info_is_dir(file)) // select filename extension for directories
    dlg.setSelectExtension(true);

  if(dlg.exec() != QDialog::Accepted)
    return;

  QString new_name = dlg.textValue();

  if(new_name == old_name)
    return;

  GFile* gf = fm_path_to_gfile(path);
  GFile* parent_gf = g_file_get_parent(gf);
  GFile* dest = g_file_get_child(G_FILE(parent_gf), new_name.toLocal8Bit().data());
  g_object_unref(parent_gf);

  GError* err = NULL;
  if(!g_file_move(gf, dest,
                  GFileCopyFlags(G_FILE_COPY_ALL_METADATA |
                                 G_FILE_COPY_NO_FALLBACK_FOR_MOVE |
                                 G_FILE_COPY_NOFOLLOW_SYMLINKS),
                  NULL, /* make this cancellable later. */
                  NULL, NULL, &err)) {
    QMessageBox::critical(parent, QObject::tr("Error"), err->message);
    g_error_free(err);
  }

  g_object_unref(dest);
  g_object_unref(gf);
}

// templateFile is a file path used as a template of the new file.
void createFile(CreateFileType type, FmPath* parentDir, FmTemplate* templ, QWidget* parent) {
  QString defaultNewName;
  QString prompt;

  switch(type) {
  case CreateNewTextFile:
    prompt = QObject::tr("Please enter a new file name:");
    defaultNewName = QObject::tr("New text file");
    break;

  case CreateNewFolder:
    prompt = QObject::tr("Please enter a new folder name:");
    defaultNewName = QObject::tr("New folder");
    break;

  case CreateWithTemplate: {
    FmMimeType* mime = fm_template_get_mime_type(templ);
    prompt = QObject::tr("Enter a name for the new %1:").arg(QString::fromUtf8(fm_mime_type_get_desc(mime)));
    defaultNewName = QString::fromUtf8(fm_template_get_name(templ, NULL));
  }
  break;
  }

_retry:
  // ask the user to input a file name
  bool ok;
  QString new_name = QInputDialog::getText(parent, QObject::tr("Create File"),
                     prompt,
                     QLineEdit::Normal,
                     defaultNewName,
                     &ok);

  if(!ok)
    return;

  GFile* parent_gf = fm_path_to_gfile(parentDir);
  GFile* dest_gf = g_file_get_child(G_FILE(parent_gf), new_name.toLocal8Bit().data());
  g_object_unref(parent_gf);

  GError* err = NULL;
  switch(type) {
  case CreateNewTextFile: {
    GFileOutputStream* f = g_file_create(dest_gf, G_FILE_CREATE_NONE, NULL, &err);
    if(f) {
      g_output_stream_close(G_OUTPUT_STREAM(f), NULL, NULL);
      g_object_unref(f);
    }
    break;
  }
  case CreateNewFolder:
    g_file_make_directory(dest_gf, NULL, &err);
    break;
  case CreateWithTemplate:
    fm_template_create_file(templ, dest_gf, &err, false);
    break;
  }
  g_object_unref(dest_gf);

  if(err) {
    if(err->domain == G_IO_ERROR && err->code == G_IO_ERROR_EXISTS) {
      g_error_free(err);
      err = NULL;
      goto _retry;
    }

    QMessageBox::critical(parent, QObject::tr("Error"), err->message);
    g_error_free(err);
  }
}

uid_t uidFromName(QString name) {
  uid_t ret;
  if(name.isEmpty())
      return -1;
  if(name.at(0).digitValue() != -1) {
    ret = uid_t(name.toUInt());
  }
  else {
    struct passwd* pw = getpwnam(name.toLatin1());
    // FIXME: use getpwnam_r instead later to make it reentrant
    ret = pw ? pw->pw_uid : -1;
  }

  return ret;
}

QString uidToName(uid_t uid) {
  QString ret;
  struct passwd* pw = getpwuid(uid);

  if(pw)
    ret = pw->pw_name;
  else
    ret = QString::number(uid);

  return ret;
}

gid_t gidFromName(QString name) {
  gid_t ret;
  if(name.isEmpty())
      return -1;
  if(name.at(0).digitValue() != -1) {
    ret = gid_t(name.toUInt());
  }
  else {
    // FIXME: use getgrnam_r instead later to make it reentrant
    struct group* grp = getgrnam(name.toLatin1());
    ret = grp ? grp->gr_gid : -1;
  }

  return ret;
}

QString gidToName(gid_t gid) {
  QString ret;
  struct group* grp = getgrgid(gid);

  if(grp)
    ret = grp->gr_name;
  else
    ret = QString::number(gid);

  return ret;
}

int execModelessDialog(QDialog* dlg) {
  // FIXME: this does much less than QDialog::exec(). Will this work flawlessly?
  QEventLoop loop;
  QObject::connect(dlg, &QDialog::finished, &loop, &QEventLoop::quit);
  // DialogExec does not seem to be documented in the Qt API doc?
  // However, in the source code of QDialog::exec(), it's used so let's use it too.
  dlg->show();
  (void)loop.exec(QEventLoop::DialogExec);
  return dlg->result();
}

// check if GVFS can support this uri scheme (lower case)
bool isUriSchemeSupported(const char* uriScheme) {
  const gchar * const * schemes = g_vfs_get_supported_uri_schemes(g_vfs_get_default());
  if(Q_UNLIKELY(schemes == NULL))
    return false;
  for(const gchar * const * scheme = schemes; *scheme; ++scheme)
    if(strcmp(uriScheme, *scheme) == 0)
      return true;
  return false;
}


};
