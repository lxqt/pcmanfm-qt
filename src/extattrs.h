// File added by probono

#ifndef EXTATTRS_H
#define EXTATTRS_H

#include <QString>

namespace Fm {
    int getAttributeValueInt(const QString& path, const QString& attribute, bool& ok);
    bool setAttributeValueInt(const QString& path, const QString& attribute, int value);
    QString getAttributeValueQString(const QString& path, const QString& attribute, bool& ok);
    bool setAttributeValueQString(const QString& path, const QString& attribute, int value);
}

#endif // EXTATTRS_H
