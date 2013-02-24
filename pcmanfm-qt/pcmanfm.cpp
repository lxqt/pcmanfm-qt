#include <libfm/fm.h>
#include "application.h"

int main(int argc, char** argv)
{
  PCManFM::Application app(argc, argv);
  return app.exec();
}
