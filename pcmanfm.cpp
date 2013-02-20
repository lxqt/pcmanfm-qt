#include <QApplication>
#include <libfm/fm.h>
#include "mainwindow.h"
#include "desktopwindow.h"
#include "application.h"

int main(int argc, char** argv)
{
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
