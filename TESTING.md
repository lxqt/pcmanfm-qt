# Testing

## GNUstep app bundle support

1. Open Filer
1. In the terminal, run: `mkdir -p Xterm.app`
1. In Filer, verify that `Xterm.app` is displayed as and behaves like a normal folder
1. In the terminal, run: `echo '#\!/bin/sh' > Xterm.app/Xterm && echo 'xterm' >> Xterm.app/Xterm`
1. In Filer, reload and verify that `Xterm.app` is still displayed as and still behaves like a normal folder
1. In the terminal, run: `chmod +x Xterm.app/Xterm`
1. In Filer, reload and verify that `Xterm` is displayed with a different icon and without the `.app` extension
1. In Filer, verify that `Xterm` in fact opens the Xterm application when double-clicked

## ROX AppDir support

1. Open Filer
1. In the terminal, run: `mkdir -p Xterm.AppDir`
1. In Filer, verify that `Xterm.app` is displayed as and behaves like a normal folder
1. In the terminal, run: `echo '#\!/bin/sh' > Xterm.AppDir/AppRun && echo 'xterm' >> Xterm.AppDir/AppRun`
1. In Filer, reload and verify that `Xterm.AppDir` is still displayed as and still behaves like a normal folder
1. In the terminal, run: `chmod +x Xterm.AppDir/AppRun`
1. In Filer, reload and verify that `Xterm` is displayed with a different icon and without the `.AppDir` extension
1. In Filer, verify that `Xterm` in fact opens the Xterm application when double-clicked
