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
#include <QPushButton>
#include <QMenu>
#include <QDir>
#include <QHash>
#include <QStringBuilder>
#include <QSettings>
#include <QStandardPaths>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QStyledItemDelegate>

#include <cmath>
#include <algorithm>

#include <libfm-qt6/folderview.h>
#include <libfm-qt6/core/terminal.h>
#include <libfm-qt6/core/archiver.h>

namespace PCManFM {

namespace {

class KeySequenceEdit : public QKeySequenceEdit {
public:
    using QKeySequenceEdit::QKeySequenceEdit;

protected:
    void keyPressEvent(QKeyEvent* event) override {
        // by not allowing multiple shortcuts,
        // the Qt bug that makes Meta a non-modifier is worked around
        clear();
        const int k = event->key();
        if(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
            if(k == Qt::Key_Super_L || k == Qt::Key_Super_R) {
                return;
            }
        }
        // keys without modifiers (or only with Shift) are used by the view, except for Fx keys and Delete
        else if((k < Qt::Key_F1 || k > Qt::Key_F35) && k != Qt::Key_Delete) {
            return;
        }
        QKeySequenceEdit::keyPressEvent(event);
    }
};

class ShortcutDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const override {
        return new KeySequenceEdit(parent);
    }
    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        static_cast<KeySequenceEdit*>(editor)->setKeySequence(QKeySequence(index.data().toString(), QKeySequence::NativeText));
    }
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
        model->setData(index, static_cast<KeySequenceEdit*>(editor)->keySequence().toString(QKeySequence::NativeText));
    }

protected:
    bool eventFilter(QObject* object, QEvent* event) override {
        QWidget* editor = qobject_cast<QWidget*>(object);
        if(editor && event->type() == QEvent::KeyPress) {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            int k = ke->key();
            // with modifiers, Return and Enter are recorded (e.g., Alt+Return)
            if((k == Qt::Key_Return || k == Qt::Key_Enter)
               && (ke->modifiers() & ~Qt::KeypadModifier) == Qt::NoModifier) {
                Q_EMIT commitData(editor);
                Q_EMIT closeEditor(editor);
                return true;
            }
        }
        return QStyledItemDelegate::eventFilter(object, event);
    }
};

} // namespace

PreferencesDialog::PreferencesDialog(const QString& activePage, QWidget* parent):
    QDialog(parent) {
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    warningCounter_ = 0;
    ui.warningLabel->hide();

    // resize the list widget according to the width of its content.
    ui.listWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ui.listWidget->setMaximumWidth(ui.listWidget->sizeHintForColumn(0) + ui.listWidget->frameWidth() * 2 + 4);

    connect(ui.terminal, &QComboBox::currentIndexChanged, this, &PreferencesDialog::terminalChanged);
    ui.terminal->lineEdit()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.terminal->lineEdit(), &QWidget::customContextMenuRequested, this, &PreferencesDialog::terminalContextMenu);
    // do not insert into the list by pressing Enter; only libfm-qt gets the terminals list
    ui.terminal->setInsertPolicy(QComboBox::NoInsert);

    initFromSettings();
    initShortcutsPage();

    selectPage(activePage);
    adjustSize();

    connect(ui.buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &PreferencesDialog::applySettings);

    if(static_cast<Application*>(qApp)->underWayland()) {
        ui.suCommand->setEnabled(false);
        ui.suLabel->setEnabled(false);
    }
}

PreferencesDialog::~PreferencesDialog() = default;

static void findIconThemesInDir(QHash<QString, QString>& iconThemes, QString dirName) {
    QDir dir(dirName);
    const QStringList subDirs = dir.entryList(QDir::AllDirs);
    GKeyFile* kf = g_key_file_new();
    for(const QString& subDir : subDirs) {
        QString indexFile = dirName + QLatin1Char('/') + subDir + QStringLiteral("/index.theme");
        if(g_key_file_load_from_file(kf, indexFile.toLocal8Bit().constData(), GKeyFileFlags(0), nullptr)) {
            // FIXME: skip hidden ones
            // icon theme must have this key, so it has icons if it has this key
            // otherwise, it might be a cursor theme or any other kind of theme.
            if(g_key_file_has_key(kf, "Icon Theme", "Directories", nullptr)) {
                char* dispName = g_key_file_get_locale_string(kf, "Icon Theme", "Name", nullptr, nullptr);
                // char* comment = g_key_file_get_locale_string(kf, "Icon Theme", "Comment", nullptr, nullptr);
                iconThemes[subDir] = QString::fromUtf8(dispName);
                g_free(dispName);
            }
        }
    }
    g_key_file_free(kf);
}

void PreferencesDialog::initIconThemes(Settings& settings) {
    // check if auto-detection is done (for example, from xsettings)
    if(settings.useFallbackIconTheme()) { // auto-detection failed
        // load xdg icon themes and select the current one
        QHash<QString, QString> iconThemes;
        // user customed icon themes
        findIconThemesInDir(iconThemes, QString::fromUtf8(g_get_home_dir()) + QStringLiteral("/.icons"));

        // search for icons in system data dir
        const char* const* dataDirs = g_get_system_data_dirs();
        for(const char* const* dataDir = dataDirs; *dataDir; ++dataDir) {
            findIconThemesInDir(iconThemes, QString::fromUtf8(*dataDir) + QStringLiteral("/icons"));
        }

        iconThemes.remove(QStringLiteral("hicolor")); // remove hicolor, which is only a fallback
        QHash<QString, QString>::const_iterator it;
        for(it = iconThemes.constBegin(); it != iconThemes.constEnd(); ++it) {
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
        if(i >= n) {
            i = 0;
        }
        ui.iconTheme->setCurrentIndex(i);
    }
    else { // auto-detection of icon theme works, hide the fallback icon theme combo box.
        ui.iconThemeLabel->hide();
        ui.iconTheme->hide();
    }

    ui.hMargin->setValue(settings.folderViewCellMargins().width());
    ui.vMargin->setValue(settings.folderViewCellMargins().height());
    connect(ui.lockMargins, &QAbstractButton::clicked, this, &PreferencesDialog::lockMargins);
}

void PreferencesDialog::initArchivers(Settings& settings) {
    auto& allArchivers = Fm::Archiver::allArchivers();
    for(int i = 0; i < int(allArchivers.size()); ++i) {
        auto& archiver = allArchivers[i];
        ui.archiver->addItem(QString::fromUtf8(archiver->program()), QString::fromUtf8(archiver->program()));
        if(QString::fromUtf8(archiver->program()) == settings.archiver()) {
            ui.archiver->setCurrentIndex(i);
        }
    }
}

void PreferencesDialog::initDisplayPage(Settings& settings) {
    initIconThemes(settings);
    // icon sizes
    int i = 0;
    for (const auto& size : Settings::iconSizes(Settings::Big)) {
        ui.bigIconSize->addItem(QStringLiteral("%1 x %1").arg(size), size);
        if(settings.bigIconSize() == size) {
            ui.bigIconSize->setCurrentIndex(i);
        }
        ++i;
    }
    i = 0;
    for (const auto& size : Settings::iconSizes(Settings::Small)) {
        QString text = QStringLiteral("%1 x %1").arg(size);
        ui.smallIconSize->addItem(text, size);
        if(settings.smallIconSize() == size) {
            ui.smallIconSize->setCurrentIndex(i);
        }

        ui.sidePaneIconSize->addItem(text, size);
        if(settings.sidePaneIconSize() == size) {
            ui.sidePaneIconSize->setCurrentIndex(i);
        }
        ++i;
    }
    i = 0;
    for (const auto& size : Settings::iconSizes(Settings::Thumbnail)) {
        ui.thumbnailIconSize->addItem(QStringLiteral("%1 x %1").arg(size), size);
        if(settings.thumbnailIconSize() == size) {
            ui.thumbnailIconSize->setCurrentIndex(i);
        }
        ++i;
    }

    ui.siUnit->setChecked(settings.siUnit());
    ui.backupAsHidden->setChecked(settings.backupAsHidden());

    ui.showFullNames->setChecked(settings.showFullNames());
    ui.shadowHidden->setChecked(settings.shadowHidden());
    ui.noItemTooltip->setChecked(settings.noItemTooltip());
    ui.noScrollPerPixel->setChecked(!settings.scrollPerPixel());

    // app restart warning
    connect(ui.showFullNames, &QAbstractButton::toggled, [this, &settings] (bool checked) {
       restartWarning(settings.showFullNames() != checked);
    });
    connect(ui.shadowHidden, &QAbstractButton::toggled, [this, &settings] (bool checked) {
       restartWarning(settings.shadowHidden() != checked);
    });
}

void PreferencesDialog::initUiPage(Settings& settings) {
    ui.alwaysShowTabs->setChecked(settings.alwaysShowTabs());
    ui.showTabClose->setChecked(settings.showTabClose());
    ui.switchToNewTab->setChecked(settings.switchToNewTab());
    ui.reopenLastTabs->setChecked(settings.reopenLastTabs());
    ui.rememberWindowSize->setChecked(settings.rememberWindowSize());
    ui.fixedWindowWidth->setValue(settings.fixedWindowWidth());
    ui.fixedWindowHeight->setValue(settings.fixedWindowHeight());
}

void PreferencesDialog::initBehaviorPage(Settings& settings) {
    ui.singleClick->setChecked(settings.singleClick());
    ui.autoSelectionDelay->setValue(double(settings.autoSelectionDelay()) / 1000);
    ui.ctrlRightClick->setChecked(settings.ctrlRightClick());

    ui.bookmarkOpenMethod->setCurrentIndex(settings.bookmarkOpenMethod());

    ui.viewMode->addItem(tr("Icon View"), (int)Fm::FolderView::IconMode);
    ui.viewMode->addItem(tr("Compact View"), (int)Fm::FolderView::CompactMode);
    ui.viewMode->addItem(tr("Thumbnail View"), (int)Fm::FolderView::ThumbnailMode);
    ui.viewMode->addItem(tr("Detailed List View"), (int)Fm::FolderView::DetailedListMode);
    const Fm::FolderView::ViewMode modes[] = {
        Fm::FolderView::IconMode,
        Fm::FolderView::CompactMode,
        Fm::FolderView::ThumbnailMode,
        Fm::FolderView::DetailedListMode
    };
    for(std::size_t i = 0; i < G_N_ELEMENTS(modes); ++i) {
        if(modes[i] == settings.viewMode()) {
            ui.viewMode->setCurrentIndex(i);
            break;
        }
    }

    ui.configmDelete->setChecked(settings.confirmDelete());

    if(settings.supportTrash()) {
        ui.useTrash->setChecked(settings.useTrash());
    }
    else {
        ui.useTrash->hide();
    }

    ui.noUsbTrash->setChecked(settings.noUsbTrash());
    ui.confirmTrash->setChecked(settings.confirmTrash());
    ui.quickExec->setChecked(settings.quickExec());
    ui.selectNewFiles->setChecked(settings.selectNewFiles());
    ui.singleWindowMode->setChecked(settings.singleWindowMode());
    ui.recentFilesSpinBox->setValue(settings.getRecentFilesNumber());

    // app restart warning
    connect(ui.quickExec, &QAbstractButton::toggled, [this, &settings] (bool checked) {
       restartWarning(settings.quickExec() != checked);
    });
}

void PreferencesDialog::initThumbnailPage(Settings& settings) {
    ui.showThumbnails->setChecked(settings.showThumbnails());
    ui.thumbnailLocal->setChecked(settings.thumbnailLocalFilesOnly());

    // the max. thumbnail size spinboxes are in MiB
    double m = settings.maxThumbnailFileSize();
    ui.maxThumbnailFileSize->setValue(std::clamp(m / 1024, 0.0, 1024.0));
    int m1 = settings.maxExternalThumbnailFileSize();
    ui.maxExternalThumbnailFileSize->setValue(m1 < 0 ? -1 : std::min(m1 / 1024, 2048));
}

void PreferencesDialog::initVolumePage(Settings& settings) {
    ui.mountOnStartup->setChecked(settings.mountOnStartup());
    ui.mountRemovable->setChecked(settings.mountRemovable());
    ui.autoRun->setChecked(settings.autoRun());
    if(settings.closeOnUnmount()) {
        ui.closeOnUnmount->setChecked(true);
    }
    else {
        ui.goHomeOnUnmount->setChecked(true);
    }
}

void PreferencesDialog::initTerminals(Settings& settings) {
    // load the known terminal list from the terminal.list file of libfm
    for(auto& terminal: Fm::allKnownTerminals()) {
        ui.terminal->addItem(QString::fromUtf8(terminal.get()));
    }
    int index = ui.terminal->findText(settings.terminal());
    ui.terminal->setCurrentIndex(index); // the options are set in "terminalChanged"
    if(index == -1) {
        ui.terminal->setEditText(settings.terminal());
    }
}

void PreferencesDialog::terminalChanged(int index) {
    // set the options if they exist in the local file
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if(!dataDir.isEmpty()) {
        const QString term = ui.terminal->itemText(index);
        QSettings termList(dataDir + QStringLiteral("/libfm-qt/terminals.list"), QSettings::IniFormat);
        if(termList.childGroups().contains(term)) {
            termList.beginGroup(term);
            ui.terminalExec->setText(termList.value(QStringLiteral("open_arg")).toString());
            ui.terminalCustom->setText(termList.value(QStringLiteral("custom_args")).toString());
            termList.endGroup();
        }
        else {
            ui.terminalExec->clear();
            ui.terminalCustom->clear();
        }
    }
}

void PreferencesDialog::terminalContextMenu(const QPoint& p) {
    QMenu menu(this);
    if(!ui.terminal->currentText().isEmpty()) {
        QAction* rmAct = menu.addAction(tr("Remove if added by user"));
        connect(rmAct, &QAction::triggered, this, [this] {
            QString term = ui.terminal->currentText();
            auto parts = term.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            if(parts.isEmpty()) {
                return;
            }
            QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
            if(dataDir.isEmpty()) {
                return;
            }
            QSettings termList(dataDir + QStringLiteral("/libfm-qt/terminals.list"), QSettings::IniFormat);
            term = parts.at(0);
            if(termList.childGroups().contains(term)) {
                termList.remove(term);
                ui.terminal->clear();
                ui.terminal->clearEditText();
                termList.sync(); // let libfm-qt pick it up here
                for(auto& terminal: Fm::allKnownTerminals()) {
                    ui.terminal->addItem(QString::fromUtf8(terminal.get()));
                }
            }
        });
    }
    QAction* openAct = menu.addAction(tr("Open user-defined list"));
    connect(openAct, &QAction::triggered, this, [] {
        QString termList = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/libfm-qt/terminals.list");
        static_cast<Application*>(qApp)->launchFiles(QDir::currentPath(), QStringList() << termList, false, false);
    });
    menu.exec(ui.terminal->lineEdit()->mapToGlobal(p));
}

void PreferencesDialog::initAdvancedPage(Settings& settings) {
    initArchivers(settings);
    initTerminals(settings);
    ui.suCommand->setText(settings.suCommand());

    ui.onlyUserTemplates->setChecked(settings.onlyUserTemplates());
    ui.templateTypeOnce->setChecked(settings.templateTypeOnce());

    ui.templateRunApp->setChecked(settings.templateRunApp());

    // FIXME: Hide options that we don't support yet.
    ui.templateRunApp->hide();

    ui.maxSearchHistory->setValue(settings.maxSearchHistory());
    ui.incrementalSearch->setChecked(settings.incrementalSearch());
}

void PreferencesDialog::initFromSettings() {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    initDisplayPage(settings);
    initUiPage(settings);
    initBehaviorPage(settings);
    initThumbnailPage(settings);
    initVolumePage(settings);
    initAdvancedPage(settings);

    connect(ui.clearSearchHistory, &QAbstractButton::clicked, this, [&settings] {
        settings.clearSearchHistory();
    });
}

void PreferencesDialog::applyDisplayPage(Settings& settings) {
    if(settings.useFallbackIconTheme()) {
        // only apply the value if icon theme combo box is in use
        // the combo box is hidden when auto-detection of icon theme from xsettings works.
        QString newIconTheme = ui.iconTheme->itemData(ui.iconTheme->currentIndex()).toString();
        if(newIconTheme != settings.fallbackIconThemeName()) {
            settings.setFallbackIconThemeName(newIconTheme);
            QIcon::setThemeName(settings.fallbackIconThemeName());
            // update the UI by emitting a style change event
            const auto widgets = QApplication::allWidgets();
            for(QWidget* widget : widgets) {
                QEvent event(QEvent::StyleChange);
                QApplication::sendEvent(widget, &event);
            }
        }
    }

    settings.setBigIconSize(ui.bigIconSize->itemData(ui.bigIconSize->currentIndex()).toInt());
    settings.setSmallIconSize(ui.smallIconSize->itemData(ui.smallIconSize->currentIndex()).toInt());
    settings.setThumbnailIconSize(ui.thumbnailIconSize->itemData(ui.thumbnailIconSize->currentIndex()).toInt());
    settings.setSidePaneIconSize(ui.sidePaneIconSize->itemData(ui.sidePaneIconSize->currentIndex()).toInt());

    settings.setSiUnit(ui.siUnit->isChecked());
    settings.setBackupAsHidden(ui.backupAsHidden->isChecked());
    settings.setShowFullNames(ui.showFullNames->isChecked());
    settings.setShadowHidden(ui.shadowHidden->isChecked());
    settings.setNoItemTooltip(ui.noItemTooltip->isChecked());
    settings.setScrollPerPixel(!ui.noScrollPerPixel->isChecked());
    settings.setFolderViewCellMargins(QSize(ui.hMargin->value(), ui.vMargin->value()));
}

void PreferencesDialog::applyUiPage(Settings& settings) {
    settings.setAlwaysShowTabs(ui.alwaysShowTabs->isChecked());
    settings.setShowTabClose(ui.showTabClose->isChecked());
    settings.setSwitchToNewTab(ui.switchToNewTab->isChecked());
    settings.setReopenLastTabs(ui.reopenLastTabs->isChecked());
    settings.setRememberWindowSize(ui.rememberWindowSize->isChecked());
    settings.setFixedWindowWidth(ui.fixedWindowWidth->value());
    settings.setFixedWindowHeight(ui.fixedWindowHeight->value());
}

void PreferencesDialog::applyBehaviorPage(Settings& settings) {
    settings.setSingleClick(ui.singleClick->isChecked());
    settings.setAutoSelectionDelay(int(ui.autoSelectionDelay->value() * 1000));
    settings.setCtrlRightClick(ui.ctrlRightClick->isChecked());

    settings.setBookmarkOpenMethod(OpenDirTargetType(ui.bookmarkOpenMethod->currentIndex()));

    // FIXME: bug here?
    Fm::FolderView::ViewMode mode = Fm::FolderView::ViewMode(ui.viewMode->itemData(ui.viewMode->currentIndex()).toInt());
    settings.setViewMode(mode);
    settings.setConfirmDelete(ui.configmDelete->isChecked());

    if(settings.supportTrash()) {
        settings.setUseTrash(ui.useTrash->isChecked());
    }

    settings.setNoUsbTrash(ui.noUsbTrash->isChecked());
    settings.setConfirmTrash(ui.confirmTrash->isChecked());
    settings.setQuickExec(ui.quickExec->isChecked());
    settings.setSelectNewFiles(ui.selectNewFiles->isChecked());
    settings.setSingleWindowMode(ui.singleWindowMode->isChecked());
    settings.setRecentFilesNumber(ui.recentFilesSpinBox->value());
}

void PreferencesDialog::applyThumbnailPage(Settings& settings) {
    settings.setShowThumbnails(ui.showThumbnails->isChecked());
    settings.setThumbnailLocalFilesOnly(ui.thumbnailLocal->isChecked());
    // the max. thumbnail size spinboxes are in MiB
    settings.setMaxThumbnailFileSize(std::round(ui.maxThumbnailFileSize->value() * 1024));
    int m = ui.maxExternalThumbnailFileSize->value();
    settings.setMaxExternalThumbnailFileSize(m < 0 ? -1 : m * 1024);
}

void PreferencesDialog::applyVolumePage(Settings& settings) {
    settings.setAutoRun(ui.autoRun->isChecked());
    settings.setMountOnStartup(ui.mountOnStartup->isChecked());
    settings.setMountRemovable(ui.mountRemovable->isChecked());
    settings.setCloseOnUnmount(ui.closeOnUnmount->isChecked());
}

void PreferencesDialog::applyTerminal(Settings& settings) {
    // set the terminal, and if needed, add it to the custom list
    QString term = ui.terminal->currentText();
    auto parts = term.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if(parts.isEmpty()) {
        return;
    }
    term = parts.at(0);
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if(!dataDir.isEmpty()) {
        bool isGlobal(false);
        for(auto& terminal: Fm::internalTerminals()) {
            if(term == QString::fromUtf8(terminal.get())) {
                isGlobal = true;
                break;
            }
        }
        QSettings termList(dataDir + QStringLiteral("/libfm-qt/terminals.list"), QSettings::IniFormat);
        if(!isGlobal
           || !ui.terminalExec->text().isEmpty()
           || !ui.terminalCustom->text().isEmpty()) {
            termList.beginGroup(term);
            termList.setValue(QStringLiteral("open_arg"), ui.terminalExec->text());
            termList.setValue(QStringLiteral("custom_args"), ui.terminalCustom->text());
            termList.endGroup();
        }
        else if(termList.childGroups().contains(term)) {
            termList.remove(term);
        }
    }
    settings.setTerminal(term);
}

void PreferencesDialog::applyAdvancedPage(Settings& settings) {
    applyTerminal(settings);
    settings.setSuCommand(ui.suCommand->text());
    settings.setArchiver(ui.archiver->itemData(ui.archiver->currentIndex()).toString());

    settings.setOnlyUserTemplates(ui.onlyUserTemplates->isChecked());
    settings.setTemplateTypeOnce(ui.templateTypeOnce->isChecked());
    settings.setTemplateRunApp(ui.templateRunApp->isChecked());

    settings.setMaxSearchHistory(ui.maxSearchHistory->value());
    settings.setIncrementalSearch(ui.incrementalSearch->isChecked());
}


void PreferencesDialog::applySettings() {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    applyDisplayPage(settings);
    applyUiPage(settings);
    applyBehaviorPage(settings);
    applyThumbnailPage(settings);
    applyVolumePage(settings);
    applyAdvancedPage(settings);
    applyShortcutsPage(settings);

    settings.save();

    Application* app = static_cast<Application*>(qApp);
    app->updateFromSettings();
}

// Column 0: action (its object name as user data), 1: shortcut (its last accepted text as user data), 2: default.
// Shortcuts are shown in the NativeText format and saved in the PortableText format.
void PreferencesDialog::initShortcutsPage() {
    Application* app = static_cast<Application*>(qApp);
    const auto& defaults = app->defaultShortcuts();
    const auto custom = app->settings().customShortcuts();
    QTableWidget* table = ui.shortcutsTable;
    table->setItemDelegateForColumn(1, new ShortcutDelegate(table));
    table->setSortingEnabled(false);
    table->setRowCount(defaults.size());
    int row = 0;
    for(auto it = defaults.constBegin(); it != defaults.constEnd(); ++it, ++row) {
        auto item = new QTableWidgetItem(it->displayText);
        item->setData(Qt::UserRole, it.key());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, 0, item);

        const QString def = it->shortcut.toString(QKeySequence::NativeText);
        const QString cur = custom.contains(it.key())
            ? QKeySequence(custom.value(it.key()), QKeySequence::PortableText).toString(QKeySequence::NativeText)
            : def;
        item = new QTableWidgetItem(cur);
        item->setData(Qt::UserRole, cur);
        table->setItem(row, 1, item);

        item = new QTableWidgetItem(def);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, 2, item);
        updateShortcutFont(row);
    }
    table->setSortingEnabled(true);
    table->sortItems(0);
    table->resizeColumnsToContents();
    ui.shortcutWarning->hide();
    updateShortcutsState(); // ambiguous shortcuts might have been added manually to the config file

    connect(table, &QTableWidget::itemChanged, this, &PreferencesDialog::onShortcutChanged);
    connect(ui.shortcutFilter, &QLineEdit::textChanged, table, [table](const QString& text) {
        for(int i = 0; i < table->rowCount(); ++i) {
            table->setRowHidden(i, !table->item(i, 0)->text().contains(text, Qt::CaseInsensitive)
                                   && !table->item(i, 1)->text().contains(text, Qt::CaseInsensitive));
        }
    });
    connect(ui.defaultShortcuts, &QAbstractButton::clicked, this, [this] {
        QTableWidget* table = ui.shortcutsTable;
        table->blockSignals(true);
        table->setSortingEnabled(false); // otherwise, rows may move while being restored
        for(int i = 0; i < table->rowCount(); ++i) {
            const QString def = table->item(i, 2)->text();
            table->item(i, 1)->setText(def);
            table->item(i, 1)->setData(Qt::UserRole, def);
            updateShortcutFont(i);
        }
        table->setSortingEnabled(true);
        table->blockSignals(false);
        updateShortcutsState();
    });
}

void PreferencesDialog::onShortcutChanged(QTableWidgetItem* item) {
    if(item->column() != 1) {
        return;
    }
    QTableWidget* table = ui.shortcutsTable;
    const QString text = item->text();
    if(!text.isEmpty() && text != table->item(item->row(), 2)->text() // a default shortcut is never reserved
       && Application::reservedShortcuts().contains(QKeySequence(text, QKeySequence::NativeText)[0].toCombined())) {
        ui.shortcutWarning->setText(tr("The typed shortcut was reserved."));
        ui.shortcutWarning->show();
        QSignalBlocker blocker(table);
        item->setText(item->data(Qt::UserRole).toString()); // restore the previous shortcut
        return;
    }
    {
        QSignalBlocker blocker(table);
        item->setData(Qt::UserRole, text);
    }
    updateShortcutFont(item->row());
    updateShortcutsState();
}

// warns about ambiguous shortcuts and enables the Default button if a shortcut is changed
void PreferencesDialog::updateShortcutsState() {
    const QTableWidget* table = ui.shortcutsTable;
    QSet<QString> shortcuts;
    bool ambiguous = false;
    bool changed = false;
    for(int i = 0; i < table->rowCount(); ++i) {
        const QString text = table->item(i, 1)->text();
        changed = changed || text != table->item(i, 2)->text();
        if(!text.isEmpty()) {
            ambiguous = ambiguous || shortcuts.contains(text);
            shortcuts << text;
        }
    }
    ui.shortcutWarning->setText(tr("<b>Warning: Ambiguous shortcut detected!</b>"));
    ui.shortcutWarning->setVisible(ambiguous);
    ui.defaultShortcuts->setEnabled(changed);
}

void PreferencesDialog::updateShortcutFont(int row) {
    QTableWidget* table = ui.shortcutsTable;
    QFont font = table->font();
    font.setBold(table->item(row, 1)->text() != table->item(row, 2)->text());
    QSignalBlocker blocker(table);
    table->item(row, 0)->setFont(font);
    table->item(row, 1)->setFont(font);
}

void PreferencesDialog::applyShortcutsPage(Settings& settings) {
    QHash<QString, QString> custom;
    const QTableWidget* table = ui.shortcutsTable;
    for(int i = 0; i < table->rowCount(); ++i) {
        const QString cur = table->item(i, 1)->text();
        if(cur != table->item(i, 2)->text()) {
            custom.insert(table->item(i, 0)->data(Qt::UserRole).toString(),
                          QKeySequence(cur, QKeySequence::NativeText).toString(QKeySequence::PortableText));
        }
    }
    settings.setCustomShortcuts(custom);
}

void PreferencesDialog::accept() {
    applySettings();
    QDialog::accept();
}

void PreferencesDialog::selectPage(const QString& name) {
    if(!name.isEmpty()) {
        QWidget* page = findChild<QWidget*>(name + QStringLiteral("Page"));
        if(page) {
            int index = ui.stackedWidget->indexOf(page);
            ui.listWidget->setCurrentRow(index);
        }
    }
}

void PreferencesDialog::lockMargins(bool lock) {
    ui.vMargin->setDisabled(lock);
    if(lock) {
        ui.vMargin->setValue(ui.hMargin->value());
        connect(ui.hMargin, &QSpinBox::valueChanged, ui.vMargin, &QSpinBox::setValue);
    }
    else {
        disconnect(ui.hMargin, &QSpinBox::valueChanged, ui.vMargin, &QSpinBox::setValue);
    }
}

void PreferencesDialog::restartWarning(bool warn) {
    if(warn) {
        ++warningCounter_;
    }
    else {
        --warningCounter_;
    }
    ui.warningLabel->setVisible(warningCounter_ > 0);
}

} // namespace PCManFM
