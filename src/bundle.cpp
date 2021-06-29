// File added by probono

#include "bundle.h"

#include <QIcon>
#include <QIcon>
#include <QDebug>
#include <QDir>

using namespace Fm;

namespace Fm {

// Return true if _info is an AppDir or .app bundle
bool checkWhetherAppDirOrBundle(FmFileInfo* _info)
{

    bool isAppDirOrBundle = false;

    // TODO: Replace by using QFileInfo that we are using anyway below; get rid of fm_...
    if((fm_file_info_is_dir(_info) || fm_file_info_is_symlink(_info))  == false) {
        return(isAppDirOrBundle);
    }

    QString path = QString(fm_path_to_str(fm_file_info_get_path(_info)));
    qDebug() << "probono: checkWhetherAppDirOrBundle" << path.toUtf8();

    QFileInfo fileInfo = QFileInfo(QDir(path).canonicalPath());
    QString nameWithoutSuffix = QFileInfo(fileInfo.completeBaseName()).fileName();

    // NOTE: Checking for the prefix speeds up things significantly,
    // e.g., when checking whether /net is an AppDir

    // Check whether we have a GNUstep .app bundle
    if (path.toLower().endsWith(".app")) {
        // TODO: Before falling back to foo.app/foo, parse the Info-gnustep.plist/Info.plist and get the NSExecutable from there
        // as described in http://www.gnustep.org/resources/documentation/Developer/Gui/ProgrammingManual/AppKit_1.html
        QFile executableFile(path.toUtf8() + "/" + nameWithoutSuffix);
        if (QFileInfo(executableFile).isExecutable()) {
            isAppDirOrBundle = true;
        }

        // Check whether we have a macOS .app bundle
        QFile infoPlistFile(path.toUtf8() + "/Contents/Info.plist");
        QFile resourcesDirectory(path.toUtf8() + "/Contents/Resources");
        if (infoPlistFile.exists() && resourcesDirectory.exists()) {
            isAppDirOrBundle = true;
        }
    }

    // Check whether we have a ROX AppDir
    if (path.toLower().endsWith(".appdir")) {
        QFile appRunFile(path.toUtf8() + "/AppRun");
        if (QFileInfo(appRunFile).isExecutable()) {
            isAppDirOrBundle = true;
        }
    }

    return(isAppDirOrBundle);
}

// Return the launchable executable of an AppDir or .app bundle
QString getLaunchableExecutable(FmFileInfo* _info)
{
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

        // macOS .app bundle
        // TODO: Before falling back to foo.app/Contents/MacOS/foo, parse Info.plist and get the CFBundleExecutable from there
        QFile executableFile2(path.toUtf8() + "/Contents/MacOS/" + nameWithoutSuffix);
        if (executableFile2.exists() && QFileInfo(executableFile2).isExecutable() && QSysInfo::productType() == "osx") {
            launchableExecutable = QString(path + "/Contents/MacOS/" + nameWithoutSuffix);
        }
    }

    // ROX AppDir
    if (path.toLower().endsWith(".appdir")) {
        QFile appRunFile(path.toUtf8() + "/AppRun");
        if ((appRunFile.exists()) && QFileInfo(appRunFile).isExecutable()) {
            launchableExecutable = QString(path + "/AppRun");
        }
    }

    qDebug() << "probono: launchableExecutable:" << launchableExecutable.toUtf8();
    return(launchableExecutable);
}

QIcon getIconForBundle(FmFileInfo* _info)
{
    QString path = QString(fm_path_to_str(fm_file_info_get_path(_info)));
    QDir appDirPath(path);
    path = appDirPath.canonicalPath(); // Resolve symlinks and get absolute path
    QIcon icon = QIcon::fromTheme("do"); // probono: In the elementary theme, this is a folder with an executable icon inside it; TODO: Find more suitable one
    QFileInfo fileInfo = QFileInfo(path);
    QString nameWithoutSuffix = QFileInfo(fileInfo.completeBaseName()).fileName();
    // probono: GNUstep .app bundle
    // http://www.gnustep.org/resources/documentation/Developer/Gui/ProgrammingManual/AppKit_1.html says:
    // To determine the icon for a folder, if the folder has a ’.app’, ’.debug’ or ’.profile’ extension - examine the Info.plist file
    // for an ’NSIcon’ value and try to use that. If there is no value specified - try foo.app/foo.tiff’ or ’foo.app/.dir.tiff’
    // TODO: Implement plist parsing. For now we just check for foo.app/Resources/foo.tiff’ and ’foo.app/.dir.tiff’
    // Actually there may be foo.app/Resources/foo.desktop files which point to Icon= and we could use that; just be sure to convert the absolute path there into a relative one?
    QFile tiffFile1(path.toUtf8() + "/Resources/" + nameWithoutSuffix.toUtf8() + ".tiff");
    if (QFile::exists((QFileInfo(tiffFile1).canonicalFilePath()))) {
        icon = QIcon(QFileInfo(tiffFile1).canonicalFilePath());
    }
    QFile tiffFile2(path.toUtf8() + "/.dir.tiff");
    if (QFile::exists((QFileInfo(tiffFile2).canonicalFilePath()))) {
        icon = QIcon(QFileInfo(tiffFile2).canonicalFilePath());
    }
    QFile pngFile1(path.toUtf8() + "/Resources/" + nameWithoutSuffix.toUtf8() + ".png");
    QFile svgFile1(path.toUtf8() + "/Resources/" + nameWithoutSuffix.toUtf8() + ".svg");
    if (QFile::exists((QFileInfo(svgFile1).canonicalFilePath()))) {
        // icon = QIcon(QFileInfo(svgFile1).canonicalFilePath());
        qDebug() << "probono: FIXME: There is a svg but we are not using it yet";
    }
    if (QFile::exists((QFileInfo(pngFile1).canonicalFilePath()))) {
        icon = QIcon(QFileInfo(pngFile1).canonicalFilePath());
    }

    // probono: ROX AppDir
    QFile dirIconFile(path.toUtf8() + "/.DirIcon");
    if (QFile::exists((QFileInfo(dirIconFile).canonicalFilePath()))) {
        icon = QIcon(QFileInfo(dirIconFile).canonicalFilePath());
    }

    // probono: macOS .app bundle
    // TODO: Implement plist parsing. For now we just check for foo.app/Contents/Resources/foo.icns’
    // TODO: Need to actually use CFBundleIconFile from Info.plist instead
    QFile icnsFile(path.toUtf8() + "/Contents/Resources/" + nameWithoutSuffix.toUtf8() + ".icns");
    if (QFile::exists((QFileInfo(icnsFile).canonicalFilePath()))) {
        icon = QIcon(QFileInfo(icnsFile).canonicalFilePath());
    }
    return(icon);
}

}
