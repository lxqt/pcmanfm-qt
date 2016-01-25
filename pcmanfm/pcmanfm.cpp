#include <libfm/fm.h>
#include "application.h"
#include <libfm-qt/libfmqt.h>

int main(int argc, char** argv) {
  // ensure that glib integration of Qt is not turned off
  // This fixes #168: https://github.com/lxde/pcmanfm-qt/issues/168
  qunsetenv("QT_NO_GLIB");

  PCManFM::Application app(argc, argv);
  app.init();
  return app.exec();
}
