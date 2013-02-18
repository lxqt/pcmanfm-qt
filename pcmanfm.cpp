#include <QtGui/QApplication>
#include "mainwindow.h"
#include <libfm/fm.h>
#include "application.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  Fm::Application fmapp;
  Fm::IconTheme::setThemeName("gnome");
  // QIcon::setThemeName("elementary");
  Fm::MainWindow mainWin;
  mainWin.resize(640, 480);
  mainWin.show();
  return app.exec();
}
