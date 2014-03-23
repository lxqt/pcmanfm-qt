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

#include "appchooserdialog.h"
#include "ui_app-chooser-dialog.h"
#include <QPushButton>
#include <gio/gdesktopappinfo.h>

namespace Fm {

AppChooserDialog::AppChooserDialog(FmMimeType* mimeType, QWidget* parent, Qt::WindowFlags f):
  QDialog(parent, f),
  mimeType_(NULL),
  selectedApp_(NULL),
  canSetDefault_(true),
  ui(new Ui::AppChooserDialog()) {
  ui->setupUi(this);

  connect(ui->appMenuView, SIGNAL(selectionChanged()), SLOT(onSelectionChanged()));
  connect(ui->tabWidget, SIGNAL(currentChanged(int)), SLOT(onTabChanged(int)));

  if(!ui->appMenuView->isAppSelected())
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false); // disable OK button

  if(mimeType)
    setMimeType(mimeType);
}

AppChooserDialog::~AppChooserDialog() {
  delete ui;
  if(mimeType_)
    fm_mime_type_unref(mimeType_);
  if(selectedApp_)
    g_object_unref(selectedApp_);
}

bool AppChooserDialog::isSetDefault() {
  return ui->setDefault->isChecked();
}


static void on_temp_appinfo_destroy(gpointer data, GObject* objptr) {
  char* filename = (char*)data;
  if(g_unlink(filename) < 0)
    g_critical("failed to remove %s", filename);
  /* else
      qDebug("temp file %s removed", filename); */
  g_free(filename);
}

static GAppInfo* app_info_create_from_commandline(const char* commandline,
    const char* application_name,
    const char* bin_name,
    const char* mime_type,
    gboolean terminal, gboolean keep) {
  GAppInfo* app = NULL;
  char* dirname = g_build_filename(g_get_user_data_dir(), "applications", NULL);
  const char* app_basename = strrchr(bin_name, '/');

  if(app_basename)
    app_basename++;
  else
    app_basename = bin_name;
  if(g_mkdir_with_parents(dirname, 0700) == 0) {
    char* filename = g_strdup_printf("%s/userapp-%s-XXXXXX.desktop", dirname, app_basename);
    int fd = g_mkstemp(filename);
    if(fd != -1) {
      GString* content = g_string_sized_new(256);
      g_string_printf(content,
                      "[" G_KEY_FILE_DESKTOP_GROUP "]\n"
                      G_KEY_FILE_DESKTOP_KEY_TYPE "=" G_KEY_FILE_DESKTOP_TYPE_APPLICATION "\n"
                      G_KEY_FILE_DESKTOP_KEY_NAME "=%s\n"
                      G_KEY_FILE_DESKTOP_KEY_EXEC "=%s\n"
                      G_KEY_FILE_DESKTOP_KEY_CATEGORIES "=Other;\n"
                      G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY "=true\n",
                      application_name,
                      commandline
                     );
      if(mime_type)
        g_string_append_printf(content,
                               G_KEY_FILE_DESKTOP_KEY_MIME_TYPE "=%s\n",
                               mime_type);
      g_string_append_printf(content,
                             G_KEY_FILE_DESKTOP_KEY_TERMINAL "=%s\n",
                             terminal ? "true" : "false");
      if(terminal)
        g_string_append_printf(content, "X-KeepTerminal=%s\n",
                               keep ? "true" : "false");
      close(fd); /* g_file_set_contents() may fail creating duplicate */
      if(g_file_set_contents(filename, content->str, content->len, NULL)) {
        char* fbname = g_path_get_basename(filename);
        app = G_APP_INFO(g_desktop_app_info_new(fbname));
        g_free(fbname);
        /* if there is mime_type set then created application will be
           saved for the mime type (see fm_choose_app_for_mime_type()
           below) but if not then we should remove this temp. file */
        if(!mime_type || !application_name[0])
          /* save the name so this file will be removed later */
          g_object_weak_ref(G_OBJECT(app), on_temp_appinfo_destroy,
                            g_strdup(filename));
      }
      else
        g_unlink(filename);
      g_string_free(content, TRUE);
    }
    g_free(filename);
  }
  g_free(dirname);
  return app;
}

inline static char* get_binary(const char* cmdline, gboolean* arg_found) {
  /* see if command line contains %f, %F, %u, or %U. */
  const char* p = strstr(cmdline, " %");
  if(p) {
    if(!strchr("fFuU", *(p + 2)))
      p = NULL;
  }
  if(arg_found)
    *arg_found = (p != NULL);
  if(p)
    return g_strndup(cmdline, p - cmdline);
  else
    return g_strdup(cmdline);
}

GAppInfo* AppChooserDialog::customCommandToApp() {
  GAppInfo* app = NULL;
  QByteArray cmdline = ui->cmdLine->text().toLocal8Bit();
  QByteArray app_name = ui->appName->text().toUtf8();
  if(!cmdline.isEmpty()) {
    gboolean arg_found = FALSE;
    char* bin1 = get_binary(cmdline.constData(), &arg_found);
    qDebug("bin1 = %s", bin1);
    /* see if command line contains %f, %F, %u, or %U. */
    if(!arg_found) {  /* append %f if no %f, %F, %u, or %U was found. */
      cmdline += " %f";
    }

    /* FIXME: is there any better way to do this? */
    /* We need to ensure that no duplicated items are added */
    if(mimeType_) {
      MenuCache* menu_cache;
      /* see if the command is already in the list of known apps for this mime-type */
      GList* apps = g_app_info_get_all_for_type(fm_mime_type_get_type(mimeType_));
      GList* l;
      for(l = apps; l; l = l->next) {
        GAppInfo* app2 = G_APP_INFO(l->data);
        const char* cmd = g_app_info_get_commandline(app2);
        char* bin2 = get_binary(cmd, NULL);
        if(g_strcmp0(bin1, bin2) == 0) {
          app = G_APP_INFO(g_object_ref(app2));
          qDebug("found in app list");
          g_free(bin2);
          break;
        }
        g_free(bin2);
      }
      g_list_foreach(apps, (GFunc)g_object_unref, NULL);
      g_list_free(apps);
      if(app)
        goto _out;

      /* see if this command can be found in menu cache */
      menu_cache = menu_cache_lookup("applications.menu");
      if(menu_cache) {
        MenuCacheDir* root_dir = menu_cache_dup_root_dir(menu_cache);
        if(root_dir) {
          GSList* all_apps = menu_cache_list_all_apps(menu_cache);
          GSList* l;
          for(l = all_apps; l; l = l->next) {
            MenuCacheApp* ma = MENU_CACHE_APP(l->data);
            const char* exec = menu_cache_app_get_exec(ma);
            char* bin2;
            if(exec == NULL) {
              g_warning("application %s has no Exec statement", menu_cache_item_get_id(MENU_CACHE_ITEM(ma)));
              continue;
            }
            bin2 = get_binary(exec, NULL);
            if(g_strcmp0(bin1, bin2) == 0) {
              app = G_APP_INFO(g_desktop_app_info_new(menu_cache_item_get_id(MENU_CACHE_ITEM(ma))));
              qDebug("found in menu cache");
              menu_cache_item_unref(MENU_CACHE_ITEM(ma));
              g_free(bin2);
              break;
            }
            menu_cache_item_unref(MENU_CACHE_ITEM(ma));
            g_free(bin2);
          }
          g_slist_free(all_apps);
          menu_cache_item_unref(MENU_CACHE_ITEM(root_dir));
        }
        menu_cache_unref(menu_cache);
      }
      if(app)
        goto _out;
    }

    /* FIXME: g_app_info_create_from_commandline force the use of %f or %u, so this is not we need */
    app = app_info_create_from_commandline(cmdline.constData(), app_name.constData(), bin1,
                                           mimeType_ ? fm_mime_type_get_type(mimeType_) : NULL,
                                           ui->useTerminal->isChecked(), ui->keepTermOpen->isChecked());
  _out:
    g_free(bin1);
  }
  return app;
}

void AppChooserDialog::accept() {
  QDialog::accept();

  if(ui->tabWidget->currentIndex() == 0) {
    selectedApp_ = ui->appMenuView->selectedApp();
  }
  else { // custom command line
    selectedApp_ = customCommandToApp();
  }

  if(selectedApp_) {
    if(mimeType_ && fm_mime_type_get_type(mimeType_) && g_app_info_get_name(selectedApp_)[0]) {
      /* add this app to the mime-type */
#if GLIB_CHECK_VERSION(2, 27, 6)
      g_app_info_set_as_last_used_for_type(selectedApp_, fm_mime_type_get_type(mimeType_), NULL);
#else
      g_app_info_add_supports_type(selectedApp_, fm_mime_type_get_type(mimeType_), NULL);
#endif
      /* if need to set default */
      if(ui->setDefault->isChecked())
        g_app_info_set_as_default_for_type(selectedApp_, fm_mime_type_get_type(mimeType_), NULL);
    }
  }
}

void AppChooserDialog::onSelectionChanged() {
  bool isAppSelected = ui->appMenuView->isAppSelected();
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isAppSelected);
}

void AppChooserDialog::setMimeType(FmMimeType* mimeType) {
  if(mimeType_)
    fm_mime_type_unref(mimeType_);

  mimeType_ = mimeType ? fm_mime_type_ref(mimeType) : NULL;
  if(mimeType_) {
    QString text = tr("Select an application to open \"%1\" files")
                   .arg(QString::fromUtf8(fm_mime_type_get_desc(mimeType_)));
    ui->fileTypeHeader->setText(text);
  }
  else {
    ui->fileTypeHeader->hide();
    ui->setDefault->hide();
  }
}

void AppChooserDialog::setCanSetDefault(bool value) {
  canSetDefault_ = value;
  ui->setDefault->setVisible(value);
}

void AppChooserDialog::onTabChanged(int index) {
  if(index == 0) { // app menu view
    onSelectionChanged();
  }
  else if(index == 1) { // custom command
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  }
}

} // namespace Fm
