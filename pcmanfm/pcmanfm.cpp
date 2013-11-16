#include <libfm/fm.h>
#include "application.h"
#include "libfmqt.h"

int main(int argc, char** argv) {
  PCManFM::Application app(argc, argv);
  app.init();
  return app.exec();
}
