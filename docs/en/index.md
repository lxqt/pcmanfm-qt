### Features


### File operations

* Hide items (without prefixing their names with a dot) ?
* Bulk rename: Select files and from Menu > Modify, Bulk rename or Cltr+F2 open bulk rename dialog
* Trusting executables: by right click on file > trust executable
* Permanently deleting files (without moving them to Trash): Shift+Delete; needs confirmation
* Open Tab as Admin: useful for transferring root files without opening a root instance
* Open Tab as Root: open root instance; check settings for Switching-User-Command in Preferences > Advanced


### Search & Filter

* Ctrl+F
* F3 opens File Search in folder
* Filter Bar (search as you type)
 

#### Custom actions

Custom actions can be created as '*.desktop files' in '~/.local/share/file-manager/actions/'

Commented example for an custom action to edit text files:

```
[Desktop Entry]
Type=Action
Icon=accessories-text-editor
#Shown as title
Name=Open as Text
#Same as above, translated
Name[es]=Abrir como texto 

#Specifies for which mimetypes action will be shown
MimeTypes=text/*;application/x-bsh;application/x-sh;text/x-script.sh;application/x-desktop;application/x-shellscript;
Profiles=profile-zero;
[X-Action-Profile profile-zero]
# Application acts on the file
Exec=featherpad %F

```

how to sort them?

* Adding/removing emblems (can be done by custom actions)?

#### Custom folder icons

From File > Folder Properties or right click on folder > properties: click the icon to change it.

#### Tab features

* Tab DND (tabs can be dragged from a window and dropped into another)
* Split view (very helpful, especially when used temporarily with F6)
* See Help > Hidden Shortcuts for Shortcuts


* Adding video, PDF and DjVu thumbnailers.


