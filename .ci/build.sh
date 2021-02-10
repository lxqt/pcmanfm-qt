set -ex

cat >> /etc/pacman.conf <<EOF
[archlinuxcn]
Server = https://repo.archlinuxcn.org/\$arch
SigLevel = Never
EOF

# Work-around the issue with glibc 2.33 on old Docker engines
# Extract files directly as pacman is also affected by the issue
patched_glibc=glibc-linux4-2.33-4-x86_64.pkg.tar.zst
curl -LO https://repo.archlinuxcn.org/x86_64/$patched_glibc
bsdtar -C / -xvf $patched_glibc

pacman -Syu --noconfirm
# See *depends in https://github.com/archlinuxcn/repo/blob/master/archlinuxcn/pcmanfm-qt-git/PKGBUILD
pacman -S --noconfirm --needed git cmake qt5-tools lxqt-build-tools-git libfm-qt-git lxmenu-data qt5-x11extras

cmake -B build -S .
make -C build
