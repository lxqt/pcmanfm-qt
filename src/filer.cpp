#include <libfm/fm.h>
#include "application.h"
#include "libfmqt.h"
#include <QDebug>
#include <QLibraryInfo>

int main(int argc, char** argv) {
  // ensure that glib integration of Qt is not turned off
  // This fixes #168: https://github.com/lxde/filer-qt/issues/168
  qunsetenv("QT_NO_GLIB");

  Filer::Application app(argc, argv);
  app.init();


  QTranslator *qtTranslator = new QTranslator(&app);
  QTranslator *translator = new QTranslator(&app);

  // Install the translations built-into Qt itself
  qDebug() << "probono: QLocale::system().name()" << QLocale::system().name();
  if (! qtTranslator->load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath))){
      // Other than qDebug, qCritical also works in Release builds
      qCritical() << "Failed qtTranslator->load";
  } else {
      if (! qApp->installTranslator(qtTranslator)){
          qCritical() << "Failed qApp->installTranslator(qtTranslator)";
      }
  }

  // Install our own translations
  if (! translator->load("filer-qt_" + QLocale::system().name(), QCoreApplication::applicationDirPath() + QString("/../share/filer/translations/"))) { // probono: FHS-like path relative to main binary
      qDebug() << "probono: loading translations from FHS tree not successful";
      if (! translator->load("filer-qt_" + QLocale::system().name(), QCoreApplication::applicationDirPath())) { // probono: When qm files are next to the executable ("uninstalled"), useful during development
          qCritical() << "Failed translator->load";
      }
  }

  if (! translator->isEmpty()) {
      if (! qApp->installTranslator(translator)){
          qCritical() << "Failed qApp->installTranslator(translator)";
      }
  }



  return app.exec();
}
