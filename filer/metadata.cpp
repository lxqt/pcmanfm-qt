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
#if defined(BSD)
#include <sys/extattr.h>
#else
#include <sys/types.h>
#include <sys/xattr.h>
#endif
#include <QDebug>

namespace {

static const int ATTR_VAL_SIZE        = 10;
static const QString WINDOW_ORIGIN_X  = "window.originx";
static const QString WINDOW_ORIGIN_Y  = "window.originy";
static const QString WINDOW_HEIGHT    = "window.height";
static const QString WINDOW_WIDTH     = "window.width";
static const QString XATTR_NAMESPACE  = "user";

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
  int val = getAttributeValueInt(path_, WINDOW_ORIGIN_X, ok);
  return val;
}

int MetaData::getWindowOriginY(bool& ok) const
{
  int val = getAttributeValueInt(path_, WINDOW_ORIGIN_Y, ok);
  return val;
}

int MetaData::getWindowHeight(bool& ok) const
{
  int val = getAttributeValueInt(path_, WINDOW_HEIGHT, ok);
  return val;
}

int MetaData::getWindowWidth(bool& ok) const
{
  int val = getAttributeValueInt(path_, WINDOW_WIDTH, ok);
  return val;
}

void MetaData::setWindowOriginX(int x)
{
  bool success = setAttributeValueInt(path_, WINDOW_ORIGIN_X, x);
  if ( ! success ) {
    qWarning() << "MetaData::setWindowOriginX: unable to set attribute";
  }
}

void MetaData::setWindowOriginY(int y)
{
  bool success = setAttributeValueInt(path_, WINDOW_ORIGIN_Y, y);
  if ( ! success ) {
    qWarning() << "MetaData::setWindowOriginX: unable to set attribute";
  }
}

void MetaData::setWindowHeight(int height)
{
  bool success = setAttributeValueInt(path_, WINDOW_HEIGHT, height);
  if ( ! success ) {
    qWarning() << "MetaData::setWindowOriginX: unable to set attribute";
  }
}

void MetaData::setWindowWidth(int width)
{
  bool success = setAttributeValueInt(path_, WINDOW_WIDTH, width);
  if ( ! success ) {
    qWarning() << "MetaData::setWindowOriginX: unable to set attribute";
  }
}
