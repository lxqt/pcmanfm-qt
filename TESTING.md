# Testing

## GNUstep `.app` bundle support

1. Open Filer
1. In the terminal, run: `mkdir -p Xterm.app`
1. In Filer, verify that `Xterm.app` is displayed as and behaves like a normal folder
1. In the terminal, run: `echo '#\!/bin/sh' > Xterm.app/Xterm && echo 'xterm' >> Xterm.app/Xterm`
1. In Filer, reload and verify that `Xterm.app` is still displayed as and still behaves like a normal folder
1. In the terminal, run: `chmod +x Xterm.app/Xterm`
1. In Filer, reload and verify that `Xterm` is displayed with a different icon and without the `.app` extension
1. In Filer, verify that `Xterm` in fact opens the Xterm application when double-clicked


## macOS `.app` bundle support

### On macOS:

1. Download https://raw.githubusercontent.com/transmission/transmission-releases/master/Transmission-3.00.dmg
1. Open the dmg to mount it
1. Open Filer
1. In Filer, go to the mountpoint of the disk image and verify that Transmission is displayed with the Transmission icon and without the `.app` extension
1. In Filer, verify that `Transmission` in fact opens the Transmission application when double-clicked

### On FreeBSD (similar for Linux):

```
su
wget -c "https://raw.githubusercontent.com/transmission/transmission-releases/master/Transmission-3.00.dmg"
pkg install dmg2img fusefs-hfsfuse
dmg2img -v /home/user/Downloads/Transmission-3.00.dmg
mdconfig -a -t vnode -f /home/user/Downloads/Transmission-3.00.img
file -s /dev/md0p1
/usr/local/bin/hfsfuse --force -o noatime /dev/md0p1 /mnt
```

1. In the terminal, run the commands above
1. Open Filer
1. In Filer, go to `/mnt` and verify that Transmission is displayed with the Transmission icon and without the `.app` extension
1. In Filer, verify that `Transmission` does not try to launch the application when double-clicked, but instead the bundle folder is opened (since we are not running on macOS)


## ROX AppDir support

1. Open Filer
1. In the terminal, run: `mkdir -p Xterm.AppDir`
1. In Filer, verify that `Xterm.app` is displayed as and behaves like a normal folder
1. In the terminal, run: `echo '#\!/bin/sh' > Xterm.AppDir/AppRun && echo 'xterm' >> Xterm.AppDir/AppRun`
1. In Filer, reload and verify that `Xterm.AppDir` is still displayed as and still behaves like a normal folder
1. In the terminal, run: `chmod +x Xterm.AppDir/AppRun`
1. In Filer, reload and verify that `Xterm` is displayed with a different icon and without the `.AppDir` extension
1. In Filer, verify that `Xterm` in fact opens the Xterm application when double-clicked
