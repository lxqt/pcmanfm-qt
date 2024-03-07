# PCManFM-Qt

## Overview

PCManFM-Qt is a Qt-based file manager which uses `GLib` for file management.
It was started as the Qt port of PCManFM, the file manager of [LXDE](https://lxde.org).

PCManFM-Qt is used by LXQt for handling the desktop. Nevertheless, it can also be used
independently of LXQt and with any desktop environment, whether under X11 or under Wayland.

PCManFM-Qt is licensed under the terms of the
[GPLv2](https://www.gnu.org/licenses/gpl-2.0.en.html) or any later version. See
file LICENSE for its full text.  

## Installation

### Compiling source code

The build dependencies are CMake, [lxqt-build-tools](https://github.com/lxqt/lxqt-build-tools),
[libfm-qt](https://github.com/lxqt/libfm-qt), and layer-shell-qt (for the desktop support under Wayland).

GVFS is an optional dependency. It provides important functionalities like Trash support.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX`
has to be set to `/usr` on most operating systems. Depending on the way library
paths are dealt with on 64bit systems, variables like `CMAKE_INSTALL_LIBDIR` may
have to be set as well.  

To build run `make`, to install `make install`, which accepts the variable `DESTDIR`
as usual.

### Binary packages

Official binary packages are available in Arch Linux, Debian,
Fedora and openSUSE (Leap and Tumbleweed) and most other distributions.

## Usage

The file manager functionality should be self-explanatory. For advanced functionalities,
see the [wiki](https://github.com/lxqt/pcmanfm-qt/wiki).

Handling of the desktop deserves some notes:

To handle the desktop, the binary `pcmanfm-qt` has to be launched with the
`--desktop` option. Optionally, `--profile` can be used for loading and
saving settings specific to certain session types, like different
desktop environments. In an LXQt session, PCManFM-Qt is launched as an
[LXQt Module](https://github.com/lxqt/lxqt-session#lxqt-modules).

The desktop can be configured by the dialog "Desktop Preferences". Technically,
it corresponds to launching `pcmanfm-qt` with the option `--desktop-pref`. It
is available in LXQt desktop's context menu and included as the "Desktop" item in
the Preferences sub-menu of LXQt Panel's main menu as well as the "LXQt Settings"
section of [Configuration Center](https://github.com/lxqt/lxqt-config#configuration-center).  

All (command-line) options are explained in detail in `man 1 pcmanfm-qt`.  

## Development

Issues should go to the tracker of PCManFM-Qt at https://github.com/lxqt/pcmanfm-qt/issues.


### Translation

Translations can be done in [LXQt-Weblate](https://translate.lxqt-project.org/projects/lxqt-desktop/pcmanfm-qt/)

<a href="https://translate.lxqt-project.org/projects/lxqt-desktop/pcmanfm-qt/">
<img src="https://translate.lxqt-project.org/widgets/lxqt-desktop/-/pcmanfm-qt/multi-auto.svg" alt="Translation status" />
</a>

