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

#include "appchoosercombobox.h"
#include "icontheme.h"
#include "appchooserdialog.h"
#include "utilities.h"

namespace Fm {

AppChooserComboBox::AppChooserComboBox(QWidget* parent):
  QComboBox(parent),
  defaultApp_(NULL),
  appInfos_(NULL),
  defaultAppIndex_(0),
  prevIndex_(0),
  mimeType_(NULL) {

  connect(this, SIGNAL(currentIndexChanged(int)), SLOT(onCurrentIndexChanged(int)));
}

AppChooserComboBox::~AppChooserComboBox() {
  if(mimeType_)
    fm_mime_type_unref(mimeType_);
  if(defaultApp_)
    g_object_unref(defaultApp_);
  // delete GAppInfo objects stored for Combobox
  if(appInfos_) {
    g_list_foreach(appInfos_, (GFunc)g_object_unref, NULL);
    g_list_free(appInfos_);
  }
}

void AppChooserComboBox::setMimeType(FmMimeType* mimeType) {
  clear();
  if(mimeType_)
    fm_mime_type_unref(mimeType_);

  mimeType_ = fm_mime_type_ref(mimeType);
  if(mimeType_) {
    const char* typeName = fm_mime_type_get_type(mimeType_);
    defaultApp_ = g_app_info_get_default_for_type(typeName, FALSE);
    appInfos_ = g_app_info_get_all_for_type(typeName);
    int i = 0;
    for(GList* l = appInfos_; l; l = l->next, ++i) {
      GAppInfo* app = G_APP_INFO(l->data);
      GIcon* gicon = g_app_info_get_icon(app);
      QString name = QString::fromUtf8(g_app_info_get_name(app));
      // QVariant data = qVariantFromValue<void*>(app);
      // addItem(IconTheme::icon(gicon), name, data);
      addItem(IconTheme::icon(gicon), name);
      if(app == defaultApp_)
        defaultAppIndex_ = i;
    }
    setCurrentIndex(defaultAppIndex_);
  }
  // add "Other applications" item
  insertSeparator(count());
  addItem(tr("Customize"));
}

// returns the currently selected app.
GAppInfo* AppChooserComboBox::selectedApp() {
  return G_APP_INFO(g_list_nth_data(appInfos_, currentIndex()));
}

bool AppChooserComboBox::isChanged() {
  return (defaultAppIndex_ != currentIndex());
}

void AppChooserComboBox::onCurrentIndexChanged(int index) {
  if(index == -1 || index == prevIndex_)
    return;

  // the last item is "Customize"
  if(index == (count() - 1)) {
    /* TODO: let the user choose an app or add custom actions here. */
    QWidget* toplevel = topLevelWidget();
    AppChooserDialog dlg(mimeType_, toplevel);
    dlg.setWindowModality(Qt::WindowModal);
    dlg.setCanSetDefault(false);
    if(dlg.exec() == QDialog::Accepted) {
      GAppInfo* app = dlg.selectedApp();
      if(app) {
        /* see if it's already in the list to prevent duplication */
        GList* found = NULL;
        for(found = appInfos_; found; found = found->next) {
          if(g_app_info_equal(app, G_APP_INFO(found->data)))
            break;
        }
        /* if it's already in the list, select it */
        if(found) {
          setCurrentIndex(g_list_position(appInfos_, found));
          g_object_unref(app);
        }
        else { /* if it's not found, add it to the list */
          appInfos_ = g_list_prepend(appInfos_, app);
          GIcon* gicon = g_app_info_get_icon(app);
          QString name = QString::fromUtf8(g_app_info_get_name(app));
          insertItem(0, IconTheme::icon(gicon), name);
          setCurrentIndex(0);
        }
        return;
      }
    }
    // restore to previously selected item
    setCurrentIndex(prevIndex_);
  }
  else {
    prevIndex_ = index;
  }
}


#if 0
/* get a list of custom apps added with app-chooser.
* the returned GList is owned by the combo box and shouldn't be freed. */
const GList* AppChooserComboBox::customApps() {

}
#endif

} // namespace Fm
