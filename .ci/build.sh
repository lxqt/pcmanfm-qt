set -ex

cat >> /etc/pacman.conf <<EOF
[archlinuxcn]
Server = https://repo.archlinuxcn.org/\$arch
SigLevel = Never
EOF

pacman -Syu --noconfirm
# See *depends in https://github.com/archlinuxcn/repo/blob/master/archlinuxcn/pcmanfm-qt-git/PKGBUILD
pacman -S --noconfirm --needed git cmake qt5-tools lxqt-build-tools-git libfm-qt-git lxmenu-data qt5-x11extras

cmake -B build -S .
make -C build
