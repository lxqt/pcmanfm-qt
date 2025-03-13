/*
    Copyright (C) 2017 Pedram Pourang (Tsu Jan) <tsujan2000@gmail.com>

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

#include "bulkrename.h"
#include <QRegularExpression>
#include <QTimer>
#include <QPushButton>
#include <QMessageBox>
#include <QProgressDialog>

#include <libfm-qt6/utilities.h>

namespace PCManFM {

BulkRenameDialog::BulkRenameDialog(QWidget* parent, Qt::WindowFlags flags) :
    QDialog(parent, flags) {
    ui.setupUi(this);
    ui.lineEdit->setFocus();
    connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this, &QDialog::accept);
    connect(ui.buttonBox->button(QDialogButtonBox::Cancel), &QAbstractButton::clicked, this, &QDialog::reject);

    // make the groupboxes mutually exclusive
    connect(ui.serialGroupBox, &QGroupBox::clicked, this, [this](bool checked) {
        if(!checked) {
            ui.serialGroupBox->setChecked(true);
        }
        ui.replaceGroupBox->setChecked(false);
        ui.caseGroupBox->setChecked(false);
    });
    connect(ui.replaceGroupBox, &QGroupBox::clicked, this, [this](bool checked) {
        if(!checked) {
            ui.replaceGroupBox->setChecked(true);
        }
        ui.serialGroupBox->setChecked(false);
        ui.caseGroupBox->setChecked(false);
    });
    connect(ui.caseGroupBox, &QGroupBox::clicked, this, [this](bool checked) {
        if(!checked) {
            ui.caseGroupBox->setChecked(true);
        }
        ui.serialGroupBox->setChecked(false);
        ui.replaceGroupBox->setChecked(false);
    });

    resize(minimumSize());
    setMaximumHeight(minimumSizeHint().height()); // no vertical resizing
}

void BulkRenameDialog::setState(const QString& baseName,
                                const QString& findStr, const QString& replaceStr,
                                bool replacement, bool caseChange,
                                bool zeroPadding, bool respectLocale, bool regex, bool toUpperCase,
                                int start, Qt::CaseSensitivity cs) {
    if(!baseName.isEmpty()) { // "Name#" by default
        ui.lineEdit->setText(baseName);
    }
    ui.spinBox->setValue(start);
    ui.zeroBox->setChecked(zeroPadding);
    ui.localeBox->setChecked(respectLocale);
    if(replacement || caseChange)
    {
        ui.serialGroupBox->setChecked(false);
        if (replacement) {
            ui.replaceGroupBox->setChecked(true);
        }
        else {
            ui.caseGroupBox->setChecked(true);
        }
    }
    ui.findLineEdit->setText(findStr);
    ui.replaceLineEdit->setText(replaceStr);
    ui.caseBox->setChecked(cs == Qt::CaseSensitive);
    ui.regexBox->setChecked(regex);
    if(toUpperCase) {
        ui.upperCaseButton->setChecked(true);
    }
    else {
        ui.lowerCaseButton->setChecked(true);
    }
}

void BulkRenameDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if(ui.lineEdit->text().endsWith(QLatin1Char('#'))) { // select what's before "#"
        QTimer::singleShot(0, this, [this]() {
            ui.lineEdit->setSelection(0, ui.lineEdit->text().size() - 1);
        });
    }
}

BulkRenamer::BulkRenamer(const Fm::FileInfoList& files, QWidget* parent) {
    if(files.size() <= 1) { // no bulk rename with just one file
        return;
    }
    bool replacement = false;
    bool caseChange = false;
    QString baseName, findStr, replaceStr;
    int start = 0;
    bool zeroPadding = false;
    bool respectLocale = false;
    bool regex = false;
    bool toUpperCase = true;
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    QLocale locale;
    bool showDlg = true;
    while(showDlg) {
        BulkRenameDialog dlg(parent);
        dlg.setState(baseName,
                     findStr, replaceStr,
                     replacement, caseChange,
                     zeroPadding, respectLocale, regex, toUpperCase,
                     start, cs);
        switch(dlg.exec()) {
        case QDialog::Accepted:
            // renaming
            baseName = dlg.getBaseName();
            start = dlg.getStart();
            zeroPadding = dlg.getZeroPadding();
            respectLocale = dlg.getRespectLocale();
            locale = dlg.locale();
            // replacement
            replacement = dlg.getReplace();
            findStr = dlg.getFindStr();
            replaceStr = dlg.getReplaceStr();
            cs = dlg.getCase();
            regex = dlg.getRegex();
            // case change
            caseChange = dlg.getCaseChange();
            toUpperCase = dlg.getUpperCase();
            locale = dlg.locale();
            break;
        default:
            return;
        }

        if(replacement) {
            showDlg = !renameByReplacing(files, findStr, replaceStr, cs, regex, parent);
        }
        else if(caseChange) {
            showDlg = !renameByChangingCase(files, locale, toUpperCase, parent);
        }
        else {
            showDlg = !rename(files, baseName, locale, start, zeroPadding, respectLocale, parent);
        }
    }
}

bool BulkRenamer::rename(const Fm::FileInfoList& files,
                         QString& baseName, const QLocale& locale,
                         int start, bool zeroPadding, bool respectLocale,
                         QWidget* parent) {
    // maximum space taken by numbers (if needed)
    int numSpace = zeroPadding ? QString::number(start + files.size()).size() : 0;
    // used for filling the space (if needed)
    const QChar zero = respectLocale ? !locale.zeroDigit().isEmpty()
                                       ? locale.zeroDigit().at(0)
                                       : QLatin1Char('0')
                                     : QLatin1Char('0');
    // used for changing numbers to strings
    const QString specifier = respectLocale ? QStringLiteral("%L1") : QStringLiteral("%1");

    if(!baseName.contains(QLatin1Char('#'))) {
        // insert "#" before the last dot
        int end = baseName.lastIndexOf(QLatin1Char('.'));
        if(end == -1) {
            end = baseName.size();
        }
        baseName.insert(end, QLatin1Char('#'));
    }
    QProgressDialog progress(QObject::tr("Renaming files..."), QObject::tr("Abort"), 0, files.size(), parent);
    progress.setWindowModality(Qt::WindowModal);
    int i = 0, failed = 0;
    const QRegularExpression extension(QStringLiteral("\\.[^.#]+$"));
    bool noExtension(baseName.indexOf(extension) == -1);
    for(auto& file: files) {
        progress.setValue(i);
        if(progress.wasCanceled()) {
            progress.close();
            QMessageBox::warning(parent, QObject::tr("Warning"), QObject::tr("Renaming is aborted."));
            return true;
        }
        // NOTE: "Edit name" seems to be the best way of handling non-UTF8 filename encoding.
        auto fileName = QString::fromUtf8(g_file_info_get_edit_name(file->gFileInfo().get()));
        if(fileName.isEmpty()) {
            fileName = QString::fromStdString(file->name());
        }
        QString newName = baseName;

        // keep the extension if the new name doesn't have one
        if(noExtension) {
            QRegularExpressionMatch match;
            if(fileName.indexOf(extension, 0, &match) > -1) {
                newName += match.captured();
            }
        }

        newName.replace(QLatin1Char('#'), specifier.arg(start + i, numSpace, 10, zero));
        if(newName == fileName || !Fm::changeFileName(file->path(), newName, nullptr, false)) {
            ++failed;
        }
        ++i;
    }
    progress.setValue(i);
    if(failed == i) {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("No file could be renamed."));
        return false;
    }
    else if(failed > 0) {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("Some files could not be renamed."));
    }
    return true;
}

bool BulkRenamer::renameByReplacing(const Fm::FileInfoList& files,
                                    const QString& findStr, const QString& replaceStr,
                                    Qt::CaseSensitivity cs, bool regex,
                                    QWidget* parent) {
    if(findStr.isEmpty()) {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("Nothing to find."));
        return false;
    }
    QRegularExpression regexFind;
    if(regex) {
        regexFind = QRegularExpression(findStr, cs == Qt::CaseSensitive
                                                  ? QRegularExpression::NoPatternOption
                                                  : QRegularExpression::CaseInsensitiveOption);
        if(!regexFind.isValid()) {
            QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("Invalid regular expression."));
            return false;
        }
    }
    QProgressDialog progress(QObject::tr("Renaming files..."), QObject::tr("Abort"), 0, files.size(), parent);
    progress.setWindowModality(Qt::WindowModal);
    int i = 0, failed = 0;
    for(auto& file: files) {
        progress.setValue(i);
        if(progress.wasCanceled()) {
            progress.close();
            QMessageBox::warning(parent, QObject::tr("Warning"), QObject::tr("Renaming is aborted."));
            return true;
        }
        auto fileName = QString::fromUtf8(g_file_info_get_edit_name(file->gFileInfo().get()));
        if(fileName.isEmpty()) {
            fileName = QString::fromStdString(file->name());
        }

        QString newName = fileName;
        if(regex) {
            newName.replace(regexFind, replaceStr);
        }
        else {
            newName.replace(findStr, replaceStr, cs);
        }
        if(newName.isEmpty() || newName == fileName
           || !Fm::changeFileName(file->path(), newName, nullptr, false)) {
            ++failed;
        }
        ++i;
    }
    progress.setValue(i);
    if(failed == i) {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("No file could be renamed."));
        return false;
    }
    else if(failed > 0) {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("Some files could not be renamed."));
    }
    return true;
}

bool BulkRenamer::renameByChangingCase(const Fm::FileInfoList& files, const QLocale& locale,
                                       bool toUpperCase, QWidget* parent) {
    QProgressDialog progress(QObject::tr("Renaming files..."), QObject::tr("Abort"), 0, files.size(), parent);
    progress.setWindowModality(Qt::WindowModal);
    int i = 0, failed = 0;
    for(auto& file: files) {
        progress.setValue(i);
        if(progress.wasCanceled()) {
            progress.close();
            QMessageBox::warning(parent, QObject::tr("Warning"), QObject::tr("Renaming is aborted."));
            return true;
        }
        auto fileName = QString::fromUtf8(g_file_info_get_edit_name(file->gFileInfo().get()));
        if(fileName.isEmpty()) {
            fileName = QString::fromStdString(file->name());
        }

        QString newName;
        if(toUpperCase){
            newName = locale.toUpper(fileName);
        }
        else {
            newName = locale.toLower(fileName);
        }
        if(newName.isEmpty() || newName == fileName
           || !Fm::changeFileName(file->path(), newName, nullptr, false)) {
            ++failed;
        }
        ++i;
    }
    progress.setValue(i);
    if(failed == i) {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("No file could be renamed."));
        return false;
    }
    else if(failed > 0) {
        QMessageBox::critical(parent, QObject::tr("Error"), QObject::tr("Some files could not be renamed."));
    }
    return true;
}

BulkRenamer::~BulkRenamer() = default;

} //namespace PCManFM
