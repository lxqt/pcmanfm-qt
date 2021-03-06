// File added by probono

#ifndef BUNDLE_H
#define BUNDLE_H

#include <QFileInfo>
#include <libfm/fm.h>

namespace Fm {
bool checkWhetherAppDirOrBundle(FmFileInfo* _info);
QString getLaunchableExecutable(FmFileInfo* _info);
QIcon getIconForBundle(FmFileInfo* _info);
}


#endif // BUNDLE_H
