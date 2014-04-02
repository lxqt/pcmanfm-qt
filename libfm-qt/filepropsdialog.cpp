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


#include "filepropsdialog.h"
#include "ui_file-props.h"
#include "icontheme.h"
#include "utilities.h"
#include "fileoperation.h"
#include <QStringBuilder>
#include <QStringListModel>
#include <QMessageBox>
#include <qdial.h>
#include <sys/types.h>
#include <time.h>

#define DIFFERENT_UIDS    ((uid)-1)
#define DIFFERENT_GIDS    ((gid)-1)
#define DIFFERENT_PERMS   ((mode_t)-1)

using namespace Fm;

enum {
  ACCESS_NO_CHANGE = 0,
  ACCESS_READ_ONLY,
  ACCESS_READ_WRITE,
  ACCESS_FORBID
};

FilePropsDialog::FilePropsDialog(FmFileInfoList* files, QWidget* parent, Qt::WindowFlags f):
  QDialog(parent, f),
  fileInfos_(fm_file_info_list_ref(files)),
  singleType(fm_file_info_list_is_same_type(files)),
  singleFile(fm_file_info_list_get_length(files) == 1 ? true:false),
  fileInfo(fm_file_info_list_peek_head(files)),
  mimeType(NULL) {

  setAttribute(Qt::WA_DeleteOnClose);

  ui = new Ui::FilePropsDialog();
  ui->setupUi(this);

  if(singleType) {
    mimeType = fm_mime_type_ref(fm_file_info_get_mime_type(fileInfo));
  }

  FmPathList* paths = fm_path_list_new_from_file_info_list(files);
  deepCountJob = fm_deep_count_job_new(paths, FM_DC_JOB_DEFAULT);
  fm_path_list_unref(paths);    

  initGeneralPage();
  initPermissionsPage();
}

FilePropsDialog::~FilePropsDialog() {
  delete ui;

  if(fileInfos_)
    fm_file_info_list_unref(fileInfos_);
  if(deepCountJob)
    g_object_unref(deepCountJob);
  if(fileSizeTimer) {
    fileSizeTimer->stop();
    delete fileSizeTimer;
    fileSizeTimer = NULL;
  }
}

void FilePropsDialog::initApplications() {
  if(singleType && mimeType && !fm_file_info_is_dir(fileInfo)) {
    ui->openWith->setMimeType(mimeType);
  }
  else {
    ui->openWith->hide();
    ui->openWithLabel->hide();
  }
}

void FilePropsDialog::initPermissionsPage() {
  // ownership handling
  // get owner/group and mode of the first file in the list
  uid = fm_file_info_get_uid(fileInfo);
  gid = fm_file_info_get_gid(fileInfo);
  mode_t mode = fm_file_info_get_mode(fileInfo);
  ownerPerm = (mode & (S_IRUSR|S_IWUSR|S_IXUSR));
  groupPerm = (mode & (S_IRGRP|S_IWGRP|S_IXGRP));
  otherPerm = (mode & (S_IROTH|S_IWOTH|S_IXOTH));
  execPerm = (mode & (S_IXUSR|S_IXGRP|S_IXOTH));
  allNative = fm_file_info_is_native(fileInfo);
  hasDir = S_ISDIR(mode);

  // check if all selected files belongs to the same owner/group or have the same mode
  // at the same time, check if all files are on native unix filesystems
  GList* l;
  for(l = fm_file_info_list_peek_head_link(fileInfos_)->next; l; l = l->next) {
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    if(allNative && !fm_file_info_is_native(fi))
      allNative = false; // not all of the files are native

    mode_t fi_mode = fm_file_info_get_mode(fi);
    if(S_ISDIR(fi_mode))
      hasDir = true; // the files list contains dir(s)

    if(uid != DIFFERENT_UIDS && uid != fm_file_info_get_uid(fi))
      uid = DIFFERENT_UIDS; // not all files have the same owner
    if(gid != DIFFERENT_GIDS && gid != fm_file_info_get_gid(fi))
      gid = DIFFERENT_GIDS; // not all files have the same owner group

    if(ownerPerm != DIFFERENT_PERMS && ownerPerm != (fi_mode & (S_IRUSR|S_IWUSR|S_IXUSR)))
      ownerPerm = DIFFERENT_PERMS; // not all files have the same permission for owner
    if(groupPerm != DIFFERENT_PERMS && groupPerm != (fi_mode & (S_IRGRP|S_IWGRP|S_IXGRP)))
      groupPerm = DIFFERENT_PERMS; // not all files have the same permission for grop
    if(otherPerm != DIFFERENT_PERMS && otherPerm != (fi_mode & (S_IROTH|S_IWOTH|S_IXOTH)))
      otherPerm = DIFFERENT_PERMS; // not all files have the same permission for other
    if(execPerm != DIFFERENT_PERMS && execPerm != (fi_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
      execPerm = DIFFERENT_PERMS; // not all files have the same executable permission
  }

  // init owner/group
  initOwner();

  // if all files are of the same type, and some of them are dirs => all of the items are dirs
  // rwx values have different meanings for dirs
  // Let's make it clear for the users
  // init combo boxes for file permissions here
  QStringList comboItems;
  comboItems.append("---"); // no change
  if(singleType && hasDir) { // all files are dirs
    comboItems.append(tr("View folder content"));
    comboItems.append(tr("View and modify folder content"));
    ui->executable->hide();
  }
  else { //not all of the files are dirs
    comboItems.append(tr("Read"));
    comboItems.append(tr("Read and write"));
  }
  comboItems.append(tr("Forbidden"));
  QStringListModel* comboModel = new QStringListModel(comboItems, this);
  ui->ownerPerm->setModel(comboModel);
  ui->groupPerm->setModel(comboModel);
  ui->otherPerm->setModel(comboModel);

  // owner
  ownerPermSel = ACCESS_NO_CHANGE;
  if(ownerPerm != DIFFERENT_PERMS) { // permissions for owner are the same among all files
    if(ownerPerm & S_IRUSR) { // can read
      if(ownerPerm & S_IWUSR) // can write
        ownerPermSel = ACCESS_READ_WRITE;
      else
        ownerPermSel = ACCESS_READ_ONLY;
    }
    else {
      if((ownerPerm & S_IWUSR) == 0) // cannot read or write
        ownerPermSel = ACCESS_FORBID;
    }
  }
  ui->ownerPerm->setCurrentIndex(ownerPermSel);

  // owner and group
  groupPermSel = ACCESS_NO_CHANGE;
  if(groupPerm != DIFFERENT_PERMS) { // permissions for owner are the same among all files
    if(groupPerm & S_IRGRP) { // can read
      if(groupPerm & S_IWGRP) // can write
        groupPermSel = ACCESS_READ_WRITE;
      else
        groupPermSel = ACCESS_READ_ONLY;
    }
    else {
      if((groupPerm & S_IWGRP) == 0) // cannot read or write
        groupPermSel = ACCESS_FORBID;
    }
  }
  ui->groupPerm->setCurrentIndex(groupPermSel);

  // other
  otherPermSel = ACCESS_NO_CHANGE;
  if(otherPerm != DIFFERENT_PERMS) { // permissions for owner are the same among all files
    if(otherPerm & S_IROTH) { // can read
      if(otherPerm & S_IWOTH) // can write
        otherPermSel = ACCESS_READ_WRITE;
      else
        otherPermSel = ACCESS_READ_ONLY;
    }
    else {
      if((otherPerm & S_IWOTH) == 0) // cannot read or write
        otherPermSel = ACCESS_FORBID;
    }

  }
  ui->otherPerm->setCurrentIndex(otherPermSel);

  // set the checkbox to partially checked state
  // when owner, group, and other have different executable flags set.
  // some of them have exec, and others do not have.
  execCheckState = Qt::PartiallyChecked;
  if(execPerm != DIFFERENT_PERMS) { // if all files have the same executable permission
    // check if the files are all executable
    if((mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == (S_IXUSR|S_IXGRP|S_IXOTH)) {
      // owner, group, and other all have exec permission.
      execCheckState = Qt::Checked;
    }
    else if((mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0) {
      // owner, group, and other all have no exec permission
      execCheckState = Qt::Unchecked;
    }
  }
  ui->executable->setCheckState(execCheckState);
}

void FilePropsDialog::initGeneralPage() {
  // update UI
  if(singleType) { // all files are of the same mime-type
    FmIcon* icon = NULL;
    // FIXME: handle custom icons for some files
    // FIXME: display special property pages for special files or
    // some specified mime-types.
    if(singleFile) { // only one file is selected.
      icon = fm_file_info_get_icon(fileInfo);
    }
    if(mimeType) {
      if(!icon) // get an icon from mime type if needed
        icon = fm_mime_type_get_icon(mimeType);
      ui->fileType->setText(QString::fromUtf8(fm_mime_type_get_desc(mimeType)));
      ui->mimeType->setText(QString::fromUtf8(fm_mime_type_get_type(mimeType)));
    }
    if(icon) {
      ui->iconButton->setIcon(IconTheme::icon(icon));
    }

    if(singleFile && fm_file_info_is_symlink(fileInfo)) {
      ui->target->setText(QString::fromUtf8(fm_file_info_get_target(fileInfo)));
    }
    else {
      ui->target->hide();
      ui->targetLabel->hide();
    }
  } // end if(singleType)
  else { // not singleType, multiple files are selected at the same time
    ui->fileType->setText(tr("Files of different types"));
    ui->target->hide();
    ui->targetLabel->hide();
  }

  // FIXME: check if all files has the same parent dir, mtime, or atime
  if(singleFile) { // only one file is selected
    FmPath* parent_path = fm_path_get_parent(fm_file_info_get_path(fileInfo));
    char* parent_str = parent_path ? fm_path_display_name(parent_path, true) : NULL;

    ui->fileName->setText(QString::fromUtf8(fm_file_info_get_disp_name(fileInfo)));
    if(parent_str) {
      ui->location->setText(QString::fromUtf8(parent_str));
      g_free(parent_str);
    }
    else
      ui->location->clear();

    ui->lastModified->setText(QString::fromUtf8(fm_file_info_get_disp_mtime(fileInfo)));

    // FIXME: need to encapsulate this in an libfm API.
    time_t atime;
    struct tm tm;
    atime = fm_file_info_get_atime(fileInfo);
    localtime_r(&atime, &tm);
    char buf[128];
    strftime(buf, sizeof(buf), "%x %R", &tm);
    ui->lastAccessed->setText(QString::fromUtf8(buf));
  }
  else {
    ui->fileName->setText(tr("Multiple Files"));
    ui->fileName->setEnabled(false);
  }

  initApplications(); // init applications combo box

  // calculate total file sizes
  fileSizeTimer = new QTimer(this);
  connect(fileSizeTimer, SIGNAL(timeout()), SLOT(onFileSizeTimerTimeout()));
  fileSizeTimer->start(600);
  g_signal_connect(deepCountJob, "finished", G_CALLBACK(onDeepCountJobFinished), this);
  fm_job_run_async(FM_JOB(deepCountJob));
}

/*static */ void FilePropsDialog::onDeepCountJobFinished(FmDeepCountJob* job, FilePropsDialog* pThis) {

  pThis->onFileSizeTimerTimeout(); // update file size display

  // free the job
  g_object_unref(pThis->deepCountJob);
  pThis->deepCountJob = NULL;

  // stop the timer
  if(pThis->fileSizeTimer) {
    pThis->fileSizeTimer->stop();
    delete pThis->fileSizeTimer;
    pThis->fileSizeTimer = NULL;
  }
}

void FilePropsDialog::onFileSizeTimerTimeout() {
  if(deepCountJob && !fm_job_is_cancelled(FM_JOB(deepCountJob))) {
    char size_str[128];
    fm_file_size_to_str(size_str, sizeof(size_str), deepCountJob->total_size,
                        fm_config->si_unit);
    // FIXME:
    // OMG! It's really unbelievable that Qt developers only implement
    // QObject::tr(... int n). GNU gettext developers are smarter and
    // they use unsigned long instead of int.
    // We cannot use Qt here to handle plural forms. So sad. :-(
    QString str = QString::fromUtf8(size_str) %
      QString(" (%1 B)").arg(deepCountJob->total_size);
      // tr(" (%n) byte(s)", "", deepCountJob->total_size);
    ui->fileSize->setText(str);

    fm_file_size_to_str(size_str, sizeof(size_str), deepCountJob->total_ondisk_size,
                        fm_config->si_unit);
    str = QString::fromUtf8(size_str) %
      QString(" (%1 B)").arg(deepCountJob->total_ondisk_size);
      // tr(" (%n) byte(s)", "", deepCountJob->total_ondisk_size);
    ui->onDiskSize->setText(str);
  }
}

void FilePropsDialog::accept() {

  // applications
  if(mimeType && ui->openWith->isChanged()) {
    GAppInfo* currentApp = ui->openWith->selectedApp();
    g_app_info_set_as_default_for_type(currentApp, fm_mime_type_get_type(mimeType), NULL);
  }

  // check if chown or chmod is needed
  guint32 newUid = uidFromName(ui->owner->text());
  guint32 newGid = gidFromName(ui->ownerGroup->text());
  bool needChown = (newUid != uid || newGid != gid);

  int newOwnerPermSel = ui->ownerPerm->currentIndex();
  int newGroupPermSel = ui->groupPerm->currentIndex();
  int newOtherPermSel = ui->otherPerm->currentIndex();
  Qt::CheckState newExecCheckState = ui->executable->checkState();
  bool needChmod = ((newOwnerPermSel != ownerPermSel) ||
                    (newGroupPermSel != groupPermSel) ||
                    (newOtherPermSel != otherPermSel) ||
                    (newExecCheckState != execCheckState));

  if(needChmod || needChown) {
    FmPathList* paths = fm_path_list_new_from_file_info_list(fileInfos_);
    FileOperation* op = new FileOperation(FileOperation::ChangeAttr, paths);
    fm_path_list_unref(paths);
    if(needChown) {
      // don't do chown if new uid/gid and the original ones are actually the same.
      if(newUid == uid)
        newUid = -1;
      if(newGid == gid)
        newGid = -1;
      op->setChown(newUid, newGid);
    }
    if(needChmod) {
      mode_t newMode = 0;
      mode_t newModeMask = 0;
      // FIXME: we need to make sure that folders with "r" permission also have "x"
      // at the same time. Otherwise, it's not able to browse the folder later.
      if(newOwnerPermSel != ownerPermSel && newOwnerPermSel != ACCESS_NO_CHANGE) {
        // owner permission changed
        newModeMask |= (S_IRUSR|S_IWUSR); // affect user bits
        if(newOwnerPermSel == ACCESS_READ_ONLY)
          newMode |= S_IRUSR;
        else if(newOwnerPermSel == ACCESS_READ_WRITE)
          newMode |= (S_IRUSR|S_IWUSR);
      }
      if(newGroupPermSel != groupPermSel && newGroupPermSel != ACCESS_NO_CHANGE) {
        qDebug("newGroupPermSel: %d", newGroupPermSel);
        // group permission changed
        newModeMask |= (S_IRGRP|S_IWGRP); // affect group bits
        if(newGroupPermSel == ACCESS_READ_ONLY)
          newMode |= S_IRGRP;
        else if(newGroupPermSel == ACCESS_READ_WRITE)
          newMode |= (S_IRGRP|S_IWGRP);
      }
      if(newOtherPermSel != otherPermSel && newOtherPermSel != ACCESS_NO_CHANGE) {
        // other permission changed
        newModeMask |= (S_IROTH|S_IWOTH); // affect other bits
        if(newOtherPermSel == ACCESS_READ_ONLY)
          newMode |= S_IROTH;
        else if(newOtherPermSel == ACCESS_READ_WRITE)
          newMode |= (S_IROTH|S_IWOTH);
      }
      if(newExecCheckState != execCheckState && newExecCheckState != Qt::PartiallyChecked) {
        // executable state changed
        newModeMask |= (S_IXUSR|S_IXGRP|S_IXOTH);
        if(newExecCheckState == Qt::Checked)
          newMode |= (S_IXUSR|S_IXGRP|S_IXOTH);
      }
      op->setChmod(newMode, newModeMask);
      
      if(hasDir) { // if there are some dirs in our selected files
        QMessageBox::StandardButton r = QMessageBox::question(this,
                                          tr("Apply changes"),
                                          tr("Do you want to recursively apply these changes to all files and sub-folders?"),
                                          QMessageBox::Yes|QMessageBox::No);
        if(r == QMessageBox::Yes)
          op->setRecursiveChattr(true);
      }
    }
    op->setAutoDestroy(true);
    op->run();
  }

  QDialog::accept();
}

void FilePropsDialog::initOwner() {
  if(allNative) {
    if(uid != DIFFERENT_UIDS)
      ui->owner->setText(uidToName(uid));
    if(gid != DIFFERENT_GIDS)
      ui->ownerGroup->setText(gidToName(gid));
    
    if(geteuid() != 0) { // on local filesystems, only root can do chown.
      ui->owner->setEnabled(false);
      ui->ownerGroup->setEnabled(false);
    }
  }
}
