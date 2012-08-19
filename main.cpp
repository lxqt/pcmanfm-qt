#include <QtGui/QApplication>
#include "demo.h"
#include <libfm/fm.h>
#include "application.h"

int main(int argc, char** argv)
{
  QIcon::setThemeName("lubuntu");
  QApplication app(argc, argv);
  Fm::Application fmapp;
  MainWindow foo;
  foo.resize(640, 480);
  foo.show();
  return app.exec();
}
