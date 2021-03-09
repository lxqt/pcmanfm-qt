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
#include <QStandardPaths>

namespace {

static const int ATTR_VAL_SIZE                  = 10;

static const QString READ_ONLY_FS_METADATA_PATH = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                                  + "/" + "filer-qt/default/metadata";

static const QString WINDOW_ORIGIN_X            = "window.originx";
static const QString WINDOW_ORIGIN_Y            = "window.originy";
static const QString WINDOW_HEIGHT              = "window.height";
static const QString WINDOW_WIDTH               = "window.width";
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
  path_(path) {
  // do nothing
}

MetaData::~MetaData() {
  // do nothing
}

int MetaData::getWindowOriginX(bool &ok) const
{
  int val = getMetadataInt(path_, WINDOW_ORIGIN_X, ok);
  return val;
}

int MetaData::getWindowOriginY(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_ORIGIN_Y, ok);
  return val;
}

int MetaData::getWindowHeight(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_HEIGHT, ok);
  return val;
}

int MetaData::getWindowWidth(bool& ok) const
{
  int val = getMetadataInt(path_, WINDOW_WIDTH, ok);
  return val;
}

void MetaData::setWindowOriginX(int x)
{
  setMetadataInt(path_, WINDOW_ORIGIN_X, x);
}

void MetaData::setWindowOriginY(int y)
{
  setMetadataInt(path_, WINDOW_ORIGIN_Y, y);
}

void MetaData::setWindowHeight(int height)
{
  setMetadataInt(path_, WINDOW_HEIGHT, height);
}

void MetaData::setWindowWidth(int width)
{
  setMetadataInt(path_, WINDOW_WIDTH, width);
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
      val = getAttributeValueInt(READ_ONLY_FS_METADATA_PATH + "/" + path, attribute, ok);
  }
  return val;
}

void MetaData::setMetadataInt(const QString& path, const QString& attribute, int value)
{
  bool success = setAttributeValueInt(path_, attribute, value);
  if ( ! success ) {
    QString mirrorPath = READ_ONLY_FS_METADATA_PATH + "/" + path;
    QDir().mkpath(mirrorPath);
    success = setAttributeValueInt(mirrorPath, attribute, value);
  }
  if ( ! success )
    qWarning() << "MetaData::setMetadataInt: unable to set attribute: " << attribute;
}
