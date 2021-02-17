set -ex

source shared-ci/prepare-archlinux.sh

# See *depends in https://github.com/archlinuxcn/repo/blob/master/archlinuxcn/pcmanfm-qt-git/PKGBUILD
pacman -S --noconfirm --needed git cmake qt5-tools lxqt-build-tools-git libfm-qt-git lxmenu-data qt5-x11extras

cmake -B build -S .
make -C build
