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


#include "preferencesdialog.h"
#include "application.h"
#include "settings.h"

using namespace PCManFM;

PreferencesDialog::PreferencesDialog (QString activePage, QWidget* parent):
  QDialog (parent) {
  ui.setupUi(this);

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

void PreferencesDialog::initIconThemes(Settings& settings) {
  // TODO: load xdg icon themes and select the current one
}

void PreferencesDialog::initArchivers(Settings& settings) {
  const GList* allArchivers = fm_archiver_get_all();
  for(const GList* l = allArchivers; l; l = l->next) {
    FmArchiver* archiver = reinterpret_cast<FmArchiver*>(l->data);
    ui.archiver->addItem(archiver->program);
  }
}

void PreferencesDialog::initUiPage(Settings& settings) {
  initIconThemes(settings);
  // icon sizes

  ui.alwaysShowTabs->setChecked(settings.alwaysShowTabs());
  // ui.hideTabClose->setChecked(settings.);
}

void PreferencesDialog::initBehaviorPage(Settings& settings) {
  ui.singleClick->setChecked(settings.singleClick());
  // ui.viewMode->setCurrentIndex(settings.viewMode());
  ui.configmDelete->setChecked(settings.confirmDelete());
  ui.useTrash->setChecked(settings.useTrash());
}

void PreferencesDialog::initThumbnailPage(Settings& settings) {
  // ui.showThumbnails->setChecked(settings.showThumbnails());
}

void PreferencesDialog::initVolumePage(Settings& settings) {
}

void PreferencesDialog::initAdvancedPage(Settings& settings) {
  initArchivers(settings);
  // ui.terminalCommand->setText(settings.terminalCommand());
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

void PreferencesDialog::applySettings() {
  Settings& settings = static_cast<Application*>(qApp)->settings();
  
}


#include "preferencesdialog.moc"
