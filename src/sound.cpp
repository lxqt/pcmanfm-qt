#include "sound.h"

#include <QCoreApplication>
#include <QFile>
#include <QtMultimedia/QSound>
#include <QDebug>

using namespace Fm;

namespace Fm {

void Fm::sound::playSound(QString wav)
{
    // probono: Play wav because it is lowest latency
    // QSound::play("://sounds/EmptyTrash.wav"); // FIXME: Cannot get this to play from a resource
    if(QFile::exists(QCoreApplication::applicationDirPath() + QString("/../share/filer/sounds/" + wav))) {
        QSound::play(QCoreApplication::applicationDirPath() + QString("/../share/filer/sounds/" + wav));
    } else if (QFile::exists(QCoreApplication::applicationDirPath() + QString("/../../src/sounds/" + wav))) {
        QSound::play(QCoreApplication::applicationDirPath() + QString("/../../src/sounds/" + wav));
    } else {
        qDebug() << "Sound not found";
    }
}

}
