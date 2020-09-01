// File added by probono

#include "bundle.h"

using namespace Fm;

namespace Fm {

// Return true if _info is an AppDir or .app bundle
bool checkWhetherAppDirOrBundle(FmFileInfo* _info)
{

    bool isAppDirOrBundle = false;

    if((fm_file_info_is_dir(_info) || fm_file_info_is_symlink(_info))  == false) {
        return(isAppDirOrBundle);
    }

    QString path = QString(fm_path_to_str(fm_file_info_get_path(_info)));
    qDebug("probono: checkWhetherAppDirOrBundle " + path.toUtf8());

    QFileInfo fileInfo = QFileInfo(path);
    QString nameWithoutSuffix = QFileInfo(fileInfo.completeBaseName()).fileName();

    // Check whether we have a GNUstep .app bundle
    if (path.toLower().endsWith(".app")) {
        // TODO: Before falling back to foo.app/foo, parse the Info-gnustep.plist/Info.plist and get the NSExecutable from there
        // as described in http://www.gnustep.org/resources/documentation/Developer/Gui/ProgrammingManual/AppKit_1.html
        QFile executableFile(path.toUtf8() + "/" + nameWithoutSuffix);
        if (executableFile.exists() && QFileInfo(executableFile).isExecutable()) {
            isAppDirOrBundle = true;
        }
    }

    // Check whether we have a macOS .app bundle
    if (path.toLower().endsWith(".app")) {
        QFile infoPlistFIle(path.toUtf8() + "/Contents/Info.plist");
        QFile resourcesDirectory(path.toUtf8() + "/Contents/Resources");
        if (infoPlistFIle.exists() && resourcesDirectory.exists()) {
            isAppDirOrBundle = true;
        }
    }

    // Check whether we have a ROX AppDir
    QFile appRunFile(path.toUtf8() + "/AppRun");
    if ((appRunFile.exists()) && QFileInfo(appRunFile).isExecutable()) {
        isAppDirOrBundle = true;
    }

    return(isAppDirOrBundle);
}

// Return the launchable executable of an AppDir or .app bundle
QString getLaunchableExecutable(FmFileInfo* _info) {

    // What to do if we found no launchableExecutable?
    // In this case we should return "nothing", but since apparently we have to return something we return the directory itself
    // TODO: Implement
    QString launchableExecutable = QString(fm_path_to_str(fm_file_info_get_path(_info)));

    QString path = QString(fm_path_to_str(fm_file_info_get_path(_info)));
    QFileInfo fileInfo = QFileInfo(path);
    QString nameWithoutSuffix = QFileInfo(fileInfo.completeBaseName()).fileName();

    // GNUstep .app bundle
    if (path.toLower().endsWith(".app")) {
        // TODO: Before falling back to foo.app/foo, parse the Info-gnustep.plist/Info.plist and get the NSExecutable from there
        // as described in http://www.gnustep.org/resources/documentation/Developer/Gui/ProgrammingManual/AppKit_1.html
        QFile executableFile(path.toUtf8() + "/" + nameWithoutSuffix);
        if (executableFile.exists() && QFileInfo(executableFile).isExecutable()) {
            launchableExecutable = QString(path + "/" + nameWithoutSuffix);
        }
    }

    // macOS .app bundle
    if (path.toLower().endsWith(".app")) {
        // TODO: Before falling back to foo.app/Contents/MacOS/foo, parse Info.plist and get the CFBundleExecutable from there
        QFile executableFile(path.toUtf8() + "/Contents/MacOS/" + nameWithoutSuffix);
        if (executableFile.exists() && QFileInfo(executableFile).isExecutable() && QSysInfo::productType() == "osx") {
            launchableExecutable = QString(path + "/Contents/MacOS/" + nameWithoutSuffix);
        }
    }

    // ROX AppDir
    QFile appRunFile(path.toUtf8() + "/AppRun");
    if ((appRunFile.exists()) && QFileInfo(appRunFile).isExecutable()) {
        launchableExecutable = QString(path + "/AppRun");
    }

    qDebug("probono: launchableExecutable: " + launchableExecutable.toUtf8());
    return(launchableExecutable);
}

}
