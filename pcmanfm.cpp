#include <QApplication>
#include <libfm/fm.h>
#include "mainwindow.h"
#include "desktopwindow.h"
#include "application.h"

int main(int argc, char** argv)
{
  // TODO: how to parse command line arguments in Qt?
  // options are:
  // 1. QCommandLine: http://xf.iksaif.net/dev/qcommandline.html
  // 2. http://code.google.com/p/qtargparser/
  
  // TODO: implement single instance
  // 1. use dbus?
  // 2. Try QtSingleApplication http://doc.qt.digia.com/solutions/4/qtsingleapplication/qtsingleapplication.html

  QApplication app(argc, argv);
  Fm::Application fmapp;
  // Fm::IconTheme::setThemeName("gnome");
  Fm::IconTheme::setThemeName("elementary");
  PCManFM::MainWindow mainWin;

  // The desktop icons window
  // PCManFM::DesktopWindow desktopWindow;
  // desktopWindow.show();

  mainWin.resize(640, 480);
  mainWin.show();
  return app.exec();
}
