#ifndef FM_DESKTOPENTRYDIALOG_H
#define FM_DESKTOPENTRYDIALOG_H

#include <QDialog>
#include "ui_desktopentrydialog.h"

#include <libfm-qt/core/filepath.h>

namespace PCManFM {

class LIBFM_QT_API DesktopEntryDialog : public QDialog {
    Q_OBJECT
public:
    explicit DesktopEntryDialog(QWidget *parent = nullptr, const Fm::FilePath& dirPath = Fm::FilePath());

    virtual ~DesktopEntryDialog();

    virtual void accept() override;

Q_SIGNALS:
    void desktopEntryCreated(const Fm::FilePath& parent, const QString& name);

private Q_SLOTS:
    void onChangingType(int type);
    void onClickingIconButton();
    void onClickingCommandButton();

private:
    Ui::DesktopEntryDialog ui;
    Fm::FilePath dirPath_;

};

} // namespace Fm
#endif // FM_DESKTOPENTRYDIALOG_H
