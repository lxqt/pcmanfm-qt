/*
 *    Copyright (C) 2021 Chris Moore <chris@mooreonline.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "metadata.h"
#include <application.h>
#include "settings.h"
#include <sys/param.h> // for checking BSD definition
#include <unistd.h>
#if defined(BSD)
#include <sys/extattr.h>
#else
#include <sys/types.h>
#include <sys/xattr.h>
#endif
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

using namespace Filer;

namespace {

static const int ATTR_VAL_SIZE                  = 10;

static const QString READ_ONLY_FS_METADATA_PATH = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                                  + "/" + "filer-qt/default/metadata";

static const QString WINDOW_ORIGIN_X            = "WindowX";
static const QString WINDOW_ORIGIN_Y            = "WindowY";
static const QString WINDOW_HEIGHT              = "WindowHeight";
static const QString WINDOW_WIDTH               = "WindowWidth";
static const QString WINDOW_VIEW                = "WindowView";
static const QString WINDOW_SORT_ITEM           = "WindowSortItem";
static const QString WINDOW_SORT_ORDER          = "WindowSortOrder";
static const QString WINDOW_SORT_CASE           = "WindowSortCase";
static const QString WINDOW_SORT_FOLDER_FIRST   = "WindowSortFolderFirst";
static const QString WINDOW_FILTER              = "WindowFilter";
static const QString XATTR_NAMESPACE            = "user";

/*
 * get the attibute value from the extended attribute for the path as int
 */
int getAttributeValueInt(const QString& path, const QString& attribute, bool& ok) {
  int value = 0;

  // get the value from the extended attribute for the path
  char data[ATTR_VAL_SIZE];
#if defined(BSD)
  ssize_t bytesRetrieved = extattr_get_file(path.toLatin1().data(), EXTATTR_NAMESPACE_USER,
                                                    attribute.toLatin1().data(), data, ATTR_VAL_SIZE);
#else
  QString namespacedAttr;
  namespacedAttr.append(XATTR_NAMESPACE).append(".").append(attribute);
  ssize_t bytesRetrieved = getxattr(path.toLatin1().data(), 
                                            namespacedAttr.toLatin1().data(), data, ATTR_VAL_SIZE);
#endif
  // check if we got the attribute value
  if (bytesRetrieved <= 0)
    ok = false;
  else {
    // convert the value to int via QString
    QString strValue(data);
    bool intOK;
    int val = strValue.toInt(&intOK);
    if (intOK) {
      ok = true;
      value = val;
    }
  }
  return value;
}

/*
 * set the attibute value in the extended attribute for the path as int
 */
bool setAttributeValueInt(const QString& path, const QString& attribute, int value) {
  // set the value from the extended attribute for the path
  QString data = QString::number(value);
#if defined(BSD)
  ssize_t bytesSet = extattr_set_file(path.toLatin1().data(), EXTATTR_NAMESPACE_USER,
                                      attribute.toLatin1().data(), data.toLatin1().data(),
                                      data.length() + 1); // include \0 termination char
  // check if we set the attribute value
  return (bytesSet > 0);
#else
  QString namespacedAttr;
  namespacedAttr.append(XATTR_NAMESPACE).append(".").append(attribute);
  int success = setxattr(path.toLatin1().data(), 
                                      namespacedAttr.toLatin1().data(), data.toLatin1().data(),
                                      data.length() + 1, 0); // include \0 termination char
  // check if we set the attribute value
  return (success == 0);
#endif
}

} // anonymous namespace

MetaData::MetaData(const QString &path):
  QObject(),
  path_(path),
  windowAttributes_(){
  loadDirInfo();
}

MetaData::~MetaData() {
  saveDirInfo();
}

int MetaData::getWindowOriginX(bool &ok) const
{
  int val = getMetadataInt(path_, WINDOW_ORIGIN_X, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_ORIGIN_X);
    val = windowAttributes_[WINDOW_ORIGIN_X].toInt();
  }
  return val;
}

int MetaData::getWindowOriginY(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_ORIGIN_Y, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_ORIGIN_Y);
    val = windowAttributes_[WINDOW_ORIGIN_Y].toInt();
  }
  return val;
}

int MetaData::getWindowHeight(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_HEIGHT, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_HEIGHT);
    val = windowAttributes_[WINDOW_HEIGHT].toInt();
  }
  return val;
}

int MetaData::getWindowWidth(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_WIDTH, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_WIDTH);
    val = windowAttributes_[WINDOW_WIDTH].toInt();
  }
  return val;
}

MetaData::FolderView MetaData::getWindowView(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_VIEW, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_VIEW);
    val = windowAttributes_[WINDOW_VIEW].toInt();
  }
  return static_cast<MetaData::FolderView>(val);
}

MetaData::SortItem MetaData::getWindowSortItem(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_SORT_ITEM, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_SORT_ITEM);
    val = windowAttributes_[WINDOW_SORT_ITEM].toInt();
  }
  return static_cast<MetaData::SortItem>(val);
}

MetaData::SortOrder MetaData::getWindowSortOrder(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_SORT_ORDER, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_SORT_ORDER);
    val = windowAttributes_[WINDOW_SORT_ORDER].toInt();
  }
  return static_cast<MetaData::SortOrder>(val);
}

MetaData::SortCase MetaData::getWindowSortCase(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_SORT_CASE, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_SORT_CASE);
    val = windowAttributes_[WINDOW_SORT_CASE].toInt();
  }
  return static_cast<MetaData::SortCase>(val);
}

MetaData::SortFolderFirst MetaData::getWindowSortFolderFirst(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_SORT_FOLDER_FIRST, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_SORT_FOLDER_FIRST);
    val = windowAttributes_[WINDOW_SORT_FOLDER_FIRST].toInt();
  }
  return static_cast<MetaData::SortFolderFirst>(val);
}

MetaData::Filter MetaData::getWindowFilter(bool &ok) const
{
  int val = getMetadataInt(path_, WINDOW_FILTER, ok);
  if (!ok) {
    ok = windowAttributes_.contains(WINDOW_FILTER);
    val = windowAttributes_[WINDOW_FILTER].toInt();
  }
  return static_cast<MetaData::Filter>(val);
}

void MetaData::setWindowOriginX(int x)
{
  setMetadataInt(path_, WINDOW_ORIGIN_X, x);
  windowAttributes_[WINDOW_ORIGIN_X] = x;
}

void MetaData::setWindowOriginY(int y)
{
  setMetadataInt(path_, WINDOW_ORIGIN_Y, y);
  windowAttributes_[WINDOW_ORIGIN_Y] = y;
}

void MetaData::setWindowHeight(int height)
{
  setMetadataInt(path_, WINDOW_HEIGHT, height);
  windowAttributes_[WINDOW_HEIGHT] = height;
}

void MetaData::setWindowWidth(int width)
{
  setMetadataInt(path_, WINDOW_WIDTH, width);
  windowAttributes_[WINDOW_WIDTH] = width;
}

void MetaData::setWindowView(MetaData::FolderView view)
{
  setMetadataInt(path_, WINDOW_VIEW, static_cast<int>(view));
  windowAttributes_[WINDOW_VIEW] = view;
}

void MetaData::setWindowSortItem(MetaData::SortItem sortItem)
{
  setMetadataInt(path_, WINDOW_SORT_ITEM, static_cast<int>(sortItem));
  windowAttributes_[WINDOW_SORT_ITEM] = sortItem;
}

void MetaData::setWindowSortOrder(MetaData::SortOrder sortOrder)
{
  setMetadataInt(path_, WINDOW_SORT_ORDER, static_cast<int>(sortOrder));
  windowAttributes_[WINDOW_SORT_ORDER] = sortOrder;
}

void MetaData::setWindowSortCase(MetaData::SortCase sortCase)
{
  setMetadataInt(path_, WINDOW_SORT_CASE, static_cast<int>(sortCase));
  windowAttributes_[WINDOW_SORT_CASE] = sortCase;
}

void MetaData::setWindowSortFolderFirst(MetaData::SortFolderFirst sortFolderFirst)
{
  setMetadataInt(path_, WINDOW_SORT_FOLDER_FIRST, static_cast<int>(sortFolderFirst));
  windowAttributes_[WINDOW_SORT_FOLDER_FIRST] = sortFolderFirst;
}

void MetaData::setWindowFilter(MetaData::Filter filter)
{
  setMetadataInt(path_, WINDOW_FILTER, static_cast<int>(filter));
  windowAttributes_[WINDOW_FILTER] = filter;
}

int MetaData::getMetadataInt(const QString& path, const QString& attribute, bool &ok) const
{
  int val = 0;
  // Check if we can write to the path - if so, use the extattrs directly, otherwise
  // read from the mirror under ~
  int result = access(path.toLatin1().data(), W_OK);
  if (result == 0) {
      val = getAttributeValueInt(path_, attribute, ok);
  }
  else {
      val = getAttributeValueInt(READ_ONLY_FS_METADATA_PATH + path, attribute, ok);
  }
  return val;
}

void MetaData::setMetadataInt(const QString& path, const QString& attribute, int value)
{
  bool success = setAttributeValueInt(path_, attribute, value);
  if ( ! success ) {
    QString mirrorPath = READ_ONLY_FS_METADATA_PATH + path;
    QDir().mkpath(mirrorPath);
    success = setAttributeValueInt(mirrorPath, attribute, value);
  }
  if ( ! success )
    qWarning() << "MetaData::setMetadataInt: unable to set attribute: " << attribute;
}

void MetaData::loadDirInfo()
{
  // Check if we can write to the path - if so, use the .DirInfo directly, otherwise
  // read from the mirror under ~
  QString path = path_;
  int result = access(path_.toLatin1().data(), W_OK);
  if (result != 0)
    path = READ_ONLY_FS_METADATA_PATH + path_;

  QFile dirInfoFile(path + "/" + ".DirInfo");
  if ( ! dirInfoFile.open(QIODevice::ReadOnly) ) {
    qDebug() << "MetaData::loadDirInfo: no .DirInfo found at path: " << path;
    return;
  }

  QByteArray dirInfoData = dirInfoFile.readAll();
  QJsonDocument dirInfoDoc(QJsonDocument::fromJson(dirInfoData));
  QJsonObject json = dirInfoDoc.object();

  if (json.contains("Window") && json["Window"].isObject())
  {
    QJsonObject windowJson = json["Window"].toObject();

    if (windowJson.contains(WINDOW_ORIGIN_X) && windowJson[WINDOW_ORIGIN_X].isDouble())
      windowAttributes_.insert(WINDOW_ORIGIN_X, windowJson[WINDOW_ORIGIN_X].toInt());

    if (windowJson.contains(WINDOW_ORIGIN_Y) && windowJson[WINDOW_ORIGIN_Y].isDouble())
      windowAttributes_.insert(WINDOW_ORIGIN_Y, windowJson[WINDOW_ORIGIN_Y].toInt());

    if (windowJson.contains(WINDOW_HEIGHT) && windowJson[WINDOW_HEIGHT].isDouble())
      windowAttributes_.insert(WINDOW_HEIGHT, windowJson[WINDOW_HEIGHT].toInt());

    if (windowJson.contains(WINDOW_WIDTH) && windowJson[WINDOW_WIDTH].isDouble())
      windowAttributes_.insert(WINDOW_WIDTH, windowJson[WINDOW_WIDTH].toInt());

    if (windowJson.contains(WINDOW_VIEW) && windowJson[WINDOW_VIEW].isDouble())
      windowAttributes_.insert(WINDOW_VIEW, windowJson[WINDOW_VIEW].toInt());

    if (windowJson.contains(WINDOW_SORT_ITEM) && windowJson[WINDOW_SORT_ITEM].isDouble())
      windowAttributes_.insert(WINDOW_SORT_ITEM, windowJson[WINDOW_SORT_ITEM].toInt());

    if (windowJson.contains(WINDOW_SORT_ORDER) && windowJson[WINDOW_SORT_ORDER].isDouble())
      windowAttributes_.insert(WINDOW_SORT_ORDER, windowJson[WINDOW_SORT_ORDER].toInt());

    if (windowJson.contains(WINDOW_SORT_CASE) && windowJson[WINDOW_SORT_CASE].isDouble())
      windowAttributes_.insert(WINDOW_SORT_CASE, windowJson[WINDOW_SORT_CASE].toInt());

    if (windowJson.contains(WINDOW_SORT_FOLDER_FIRST) && windowJson[WINDOW_SORT_FOLDER_FIRST].isDouble())
      windowAttributes_.insert(WINDOW_SORT_FOLDER_FIRST, windowJson[WINDOW_SORT_FOLDER_FIRST].toInt());

    if (windowJson.contains(WINDOW_FILTER) && windowJson[WINDOW_FILTER].isDouble())
      windowAttributes_.insert(WINDOW_FILTER, windowJson[WINDOW_FILTER].toInt());
  }
}

void MetaData::saveDirInfo()
{
  // Don't write if user turned off the option in preferences
  Settings& settings = static_cast<Application*>(qApp)->settings();
  if ( ! settings.dirInfoWrite() )
    return;

  // Don't write if there's a flag in the volume root directory
  if (dirInfoDisabledForPath()) {
    qDebug() << "MetaData::saveDirInfo: found a .DisableDirInfo - not writing";
    return;
  }

  QFile dirInfoFile(path_ + "/" + ".DirInfo");
  if ( ! dirInfoFile.open(QIODevice::WriteOnly) ) {
    qDebug() << "MetaData::saveDirInfo: could not open .DirInfo to write at path: " << path_
             << ", writing to mirror under ~";
    QString mirrorPath = READ_ONLY_FS_METADATA_PATH + path_;
    QDir().mkpath(mirrorPath);
    dirInfoFile.setFileName(mirrorPath + "/" + ".DirInfo");
    if ( ! dirInfoFile.open(QIODevice::WriteOnly) ) {
      qWarning() << "MetaData::saveDirInfo: could not open .DirInfo to write at path: " << mirrorPath;
      return;
    }
  }

  QJsonObject json;
  QJsonObject windowJson;

  if (windowAttributes_.contains(WINDOW_ORIGIN_X))
    windowJson[WINDOW_ORIGIN_X] = windowAttributes_[WINDOW_ORIGIN_X].toInt();

  if (windowAttributes_.contains(WINDOW_ORIGIN_Y))
    windowJson[WINDOW_ORIGIN_Y] = windowAttributes_[WINDOW_ORIGIN_Y].toInt();

  if (windowAttributes_.contains(WINDOW_HEIGHT))
    windowJson[WINDOW_HEIGHT] = windowAttributes_[WINDOW_HEIGHT].toInt();

  if (windowAttributes_.contains(WINDOW_WIDTH))
    windowJson[WINDOW_WIDTH] = windowAttributes_[WINDOW_WIDTH].toInt();

  if (windowAttributes_.contains(WINDOW_VIEW))
    windowJson[WINDOW_VIEW] = windowAttributes_[WINDOW_VIEW].toInt();

  if (windowAttributes_.contains(WINDOW_SORT_ITEM))
    windowJson[WINDOW_SORT_ITEM] = windowAttributes_[WINDOW_SORT_ITEM].toInt();

  if (windowAttributes_.contains(WINDOW_SORT_ORDER))
    windowJson[WINDOW_SORT_ORDER] = windowAttributes_[WINDOW_SORT_ORDER].toInt();

  if (windowAttributes_.contains(WINDOW_SORT_CASE))
    windowJson[WINDOW_SORT_CASE] = windowAttributes_[WINDOW_SORT_CASE].toInt();

  if (windowAttributes_.contains(WINDOW_SORT_FOLDER_FIRST))
    windowJson[WINDOW_SORT_FOLDER_FIRST] = windowAttributes_[WINDOW_SORT_FOLDER_FIRST].toInt();

  if (windowAttributes_.contains(WINDOW_FILTER))
    windowJson[WINDOW_FILTER] = windowAttributes_[WINDOW_FILTER].toInt();

  json["Window"] = windowJson;
  QJsonDocument dirInfoDoc(json);
  dirInfoFile.write(dirInfoDoc.toJson());
}

bool MetaData::dirInfoDisabledForPath() const
{
  // check for a .DisableDirInfo file in the path
  QStringList splitPath = QDir::toNativeSeparators(path_).split(QDir::separator(), Qt::SkipEmptyParts);
  QString walkedPath = "/";
  for (QString directory : splitPath) {
    walkedPath.append(directory + "/");
    QFile file(walkedPath + "/" + ".DisableDirInfo");
    if (file.exists())
        return true;
  }
  return false;
}
