#include "dbusinterface.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>

DBusInterface::DBusInterface() :
    QObject()
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/freedesktop/FileManager1"), this,
            QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);
    QDBusConnection::sessionBus().interface()->registerService(QStringLiteral("org.freedesktop.FileManager1"),
                                                               QDBusConnectionInterface::QueueService);
}

void DBusInterface::ShowFolders(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(uriList)
    Q_UNUSED(startUpId)
    qDebug("TODO: Implement ShowFolders");
}

void DBusInterface::ShowItems(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(uriList)
    Q_UNUSED(startUpId)
    qDebug("TODO: Implement ShowItems");
}

void DBusInterface::ShowItemProperties(const QStringList& uriList, const QString& startUpId)
{
    Q_UNUSED(uriList)
    Q_UNUSED(startUpId)
    qDebug("TODO: Implement ShowItemProperties");
}
