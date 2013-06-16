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


#include "preferencesdialog.h"
#include "application.h"
#include "settings.h"
#include <QDir>
#include <QHash>
#include <QStringBuilder>

#include "folderview.h"

using namespace PCManFM;

static int bigIconSizes[] = {96, 72, 64, 48, 36, 32, 24, 20};
static int smallIconSizes[] = {48, 36, 32, 24, 20, 16, 12};
static int thumbnailIconSizes[] = {256, 224, 192, 160, 128, 96, 64};

PreferencesDialog::PreferencesDialog (QString activePage, QWidget* parent):
  QDialog (parent) {
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);

  initFromSettings();
  
  if(!activePage.isEmpty()) {
    QWidget* page = findChild<QWidget*>(activePage + "Page");
    if(page) {
      int index = ui.stackedWidget->indexOf(page);
      ui.listWidget->setCurrentRow(index);
    }
  }
}

PreferencesDialog::~PreferencesDialog() {

}

static void findIconThemesInDir(QHash<QString, QString>& iconThemes, QString dirName) {
  QDir dir(dirName);
  QStringList subDirs = dir.entryList(QDir::AllDirs);
  GKeyFile* kf = g_key_file_new();
  Q_FOREACH(QString subDir, subDirs) {
    QString indexFile = dirName % '/' % subDir % "/index.theme";
    if(g_key_file_load_from_file(kf, indexFile.toLocal8Bit().constData(), GKeyFileFlags(0), NULL)) {
      // FIXME: skip hidden ones
      // icon theme must have this key, so it has icons if it has this key
      // otherwise, it might be a cursor theme or any other kind of theme.
      if(g_key_file_has_key(kf, "Icon Theme", "Directories", NULL)) {
        char* dispName = g_key_file_get_locale_string(kf, "Icon Theme", "Name", NULL, NULL);
        // char* comment = g_key_file_get_locale_string(kf, "Icon Theme", "Comment", NULL, NULL);
        iconThemes[subDir] = dispName;
        g_free(dispName);
      }
    }
  }
  g_key_file_free(kf);
}

void PreferencesDialog::initIconThemes(Settings& settings) {
  Application* app = static_cast<Application*>(qApp);

  // check if auto-detection is done (for example, from xsettings)
  if(app->desktopSettings().iconThemeName().isEmpty()) { // auto-detection failed
    // load xdg icon themes and select the current one
    QHash<QString, QString> iconThemes;
    // user customed icon themes
    findIconThemesInDir(iconThemes, QString(g_get_home_dir()) % "/.icons");

    // search for icons in system data dir
    const char* const* dataDirs = g_get_system_data_dirs();
    for(const char* const* dataDir = dataDirs; *dataDir; ++dataDir) {
      findIconThemesInDir(iconThemes, QString(*dataDir) % "/icons");
    }

    iconThemes.remove("hicolor"); // remove hicolor, which is only a fallback
    QHash<QString, QString>::const_iterator it;
    for(it = iconThemes.begin(); it != iconThemes.end(); ++it) {
      ui.iconTheme->addItem(it.value(), it.key());
    }
    ui.iconTheme->model()->sort(0); // sort the list of icon theme names

    // select current theme name
    int n = ui.iconTheme->count();
    int i;
    for(i = 0; i < n; ++i) {
      QVariant itemData = ui.iconTheme->itemData(i);
      if(itemData == settings.fallbackIconThemeName()) {
	break;
      }
    }
    if(i >= n)
      i = 0;
    ui.iconTheme->setCurrentIndex(i);
  }
  else { // auto-detection of icon theme works, hide the fallback icon theme combo box.
    ui.iconThemeLabel->hide();
    ui.iconTheme->hide();
  }
}

void PreferencesDialog::initArchivers(Settings& settings) {
  const GList* allArchivers = fm_archiver_get_all();
  int i = 0;
  for(const GList* l = allArchivers; l; l = l->next, ++i) {
    FmArchiver* archiver = reinterpret_cast<FmArchiver*>(l->data);
    ui.archiver->addItem(archiver->program, QString(archiver->program));
    if(archiver->program == settings.archiver())
      ui.archiver->setCurrentIndex(i);
  }
}

void PreferencesDialog::initUiPage(Settings& settings) {
  initIconThemes(settings);
  // icon sizes
  int i;
  for(i = 0; i < G_N_ELEMENTS(bigIconSizes); ++i) {
    int size = bigIconSizes[i];
    ui.bigIconSize->addItem(QString("%1 x %1").arg(size), size);
    if(settings.bigIconSize() == size)
      ui.bigIconSize->setCurrentIndex(i);
  }
  for(i = 0; i < G_N_ELEMENTS(smallIconSizes); ++i) {
    int size = smallIconSizes[i];
    QString text = QString("%1 x %1").arg(size);
    ui.smallIconSize->addItem(text, size);
    if(settings.smallIconSize() == size)
      ui.smallIconSize->setCurrentIndex(i);

    ui.sidePaneIconSize->addItem(text, size);
    if(settings.sidePaneIconSize() == size)
      ui.sidePaneIconSize->setCurrentIndex(i);
  }
  for(i = 0; i < G_N_ELEMENTS(thumbnailIconSizes); ++i) {
    int size = thumbnailIconSizes[i];
    ui.thumbnailIconSize->addItem(QString("%1 x %1").arg(size), size);
    if(settings.thumbnailIconSize() == size)
      ui.thumbnailIconSize->setCurrentIndex(i);
  }

  ui.alwaysShowTabs->setChecked(settings.alwaysShowTabs());
  ui.showTabClose->setChecked(settings.showTabClose());
  ui.windowWidth->setValue(settings.windowWidth());
  ui.windowHeight->setValue(settings.windowHeight());
}

void PreferencesDialog::initBehaviorPage(Settings& settings) {
  ui.singleClick->setChecked(settings.singleClick());

  ui.viewMode->addItem(tr("Icon View"), (int)Fm::FolderView::IconMode);
  ui.viewMode->addItem(tr("Compact Icon View"), (int)Fm::FolderView::CompactMode);
  ui.viewMode->addItem(tr("Thumbnail View"), (int)Fm::FolderView::ThumbnailMode);
  ui.viewMode->addItem(tr("Detailed List View"), (int)Fm::FolderView::DetailedListMode);
  Fm::FolderView::ViewMode modes[] = {
    Fm::FolderView::IconMode,
    Fm::FolderView::CompactMode,
    Fm::FolderView::ThumbnailMode,
    Fm::FolderView::DetailedListMode   
  };
  for(int i = 0; i < G_N_ELEMENTS(modes); ++i) {
    if(modes[i] = settings.viewMode()) {
      ui.viewMode->setCurrentIndex(i);
      break;
    }
  } 

  // ui.viewMode->setCurrentIndex(settings.viewMode());
  ui.configmDelete->setChecked(settings.confirmDelete());
  ui.useTrash->setChecked(settings.useTrash());
}

void PreferencesDialog::initThumbnailPage(Settings& settings) {
  ui.showThumbnails->setChecked(settings.showThumbnails());
  ui.thumbnailLocal->setChecked(settings.thumbnailLocalFilesOnly());
  ui.maxThumbnailFileSize->setValue(settings.maxThumbnailFileSize());
}

void PreferencesDialog::initVolumePage(Settings& settings) {
  ui.mountOnStartup->setChecked(settings.mountOnStartup());
  ui.mountRemovable->setChecked(settings.mountRemovable());
  ui.autoRun->setChecked(settings.autoRun());
}

void PreferencesDialog::initAdvancedPage(Settings& settings) {
  initArchivers(settings);
  ui.terminalDirCommand->setText(settings.terminalDirCommand());
  ui.terminalExecCommand->setText(settings.terminalExecCommand());
  ui.suCommand->setText(settings.suCommand());
  // ui.siUnit->setChecked(settings.siUnit());
}

void PreferencesDialog::initFromSettings() {
  Settings& settings = static_cast<Application*>(qApp)->settings();
  initUiPage(settings);
  initBehaviorPage(settings);
  initThumbnailPage(settings);
  initVolumePage(settings);
  initAdvancedPage(settings);
}

void PreferencesDialog::applyUiPage(Settings& settings) {
  if(ui.iconTheme->isVisible()) {
    // only apply the value if icon theme combo box is in use
    // the combo box is hidden when auto-detection of icon theme from xsettings works.
    settings.setFallbackIconThemeName(ui.iconTheme->itemData(ui.iconTheme->currentIndex()).toString());
  }
  settings.setBigIconSize(ui.bigIconSize->itemData(ui.bigIconSize->currentIndex()).toInt());
  settings.setSmallIconSize(ui.smallIconSize->itemData(ui.smallIconSize->currentIndex()).toInt());
  settings.setThumbnailIconSize(ui.thumbnailIconSize->itemData(ui.thumbnailIconSize->currentIndex()).toInt());
  settings.setSidePaneIconSize(ui.sidePaneIconSize->itemData(ui.sidePaneIconSize->currentIndex()).toInt());
  settings.setAlwaysShowTabs(ui.alwaysShowTabs->isChecked());
  settings.setShowTabClose(ui.showTabClose->isChecked());
  settings.setWindowWidth(ui.windowWidth->value());
  settings.setWindowHeight(ui.windowHeight->value());
}

void PreferencesDialog::applyBehaviorPage(Settings& settings) {
  settings.setSingleClick(ui.singleClick->isChecked());
  // FIXME: bug here?
  Fm::FolderView::ViewMode mode = Fm::FolderView::ViewMode(ui.viewMode->itemData(ui.viewMode->currentIndex()).toInt());
  settings.setViewMode(mode);
  settings.setConfirmDelete(ui.configmDelete->isChecked());
  settings.setUseTrash(ui.useTrash->isChecked());
}

void PreferencesDialog::applyThumbnailPage(Settings& settings) {
  settings.setShowThumbnails(ui.showThumbnails->isChecked());
  settings.setThumbnailLocalFilesOnly(ui.thumbnailLocal->isChecked());
  settings.setMaxThumbnailFileSize(ui.maxThumbnailFileSize->value());
}

void PreferencesDialog::applyVolumePage(Settings& settings) {
  settings.setAutoRun(ui.autoRun->isChecked());
  settings.setMountOnStartup(ui.mountOnStartup->isChecked());
  settings.setMountRemovable(ui.mountRemovable->isChecked());
}

void PreferencesDialog::applyAdvancedPage(Settings& settings) {
  settings.setTerminalDirCommand(ui.terminalDirCommand->text());
  settings.setTerminalExecCommand(ui.terminalExecCommand->text());
  settings.setSuCommand(ui.suCommand->text());
  settings.setArchiver(ui.archiver->itemData(ui.archiver->currentIndex()).toString());
  settings.setSiUnit(ui.siUnit->isChecked());
}


void PreferencesDialog::applySettings() {
  Settings& settings = static_cast<Application*>(qApp)->settings();
  applyUiPage(settings);
  applyBehaviorPage(settings);
  applyThumbnailPage(settings);
  applyVolumePage(settings);
  applyAdvancedPage(settings);

  settings.save();
  
  Application* app = static_cast<Application*>(qApp);
  app->updateFromSettings();
}

void PreferencesDialog::accept() {
  applySettings();
  QDialog::accept();
}

