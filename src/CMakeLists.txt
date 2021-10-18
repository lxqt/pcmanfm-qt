# set(LIBFM_LIBRARY "fm-qt5")
set(QTX_INCLUDE_DIRS "")
set(QTX_LIBRARIES Qt5::Widgets Qt5::DBus Qt5::X11Extras)

include_directories(
    ${QTX_INCLUDE_DIRS}
    ${LIBFM_INCLUDE_DIRS}
    "${LIBFM_INCLUDEDIR}/libfm" # to workaround incorrect #include in fm-actions.
    ${LIBMENUCACHE_INCLUDE_DIRS}
    ${SYSTEM_LIBS_INCLUDE_DIRS}
    "${CMAKE_CURRENT_BINARY_DIR}"
)

link_directories(
    ${LIBMENUCACHE_LIBRARY_DIRS}
    ${SYSTEM_LIBS_LIBRARY_DIRS}
)

set(filer_SRCS
    bundle.cpp # probono
    libfmqt.cpp
    bookmarkaction.cpp
    sidepane.cpp
    icontheme.cpp
    filelauncher.cpp
    foldermodel.cpp
    foldermodelitem.cpp
    cachedfoldermodel.cpp
    proxyfoldermodel.cpp
    folderview.cpp
    folderitemdelegate.cpp
    createnewmenu.cpp
    filemenu.cpp
    foldermenu.cpp
    filepropsdialog.cpp
    applaunchcontext.cpp
    placesview.cpp
    placesmodel.cpp
    placesmodelitem.cpp
    dirtreeview.cpp
    dirtreemodel.cpp
    dirtreemodelitem.cpp
    dnddest.cpp
    mountoperation.cpp
    mountoperationpassworddialog.cpp
    mountoperationquestiondialog.cpp
    fileoperation.cpp
    fileoperationdialog.cpp
    renamedialog.cpp
    pathedit.cpp
    colorbutton.cpp
    fontbutton.cpp
    browsehistory.cpp
    utilities.cpp
    dndactionmenu.cpp
    editbookmarksdialog.cpp
    thumbnailloader.cpp
    path.cpp
    execfiledialog.cpp
    appchoosercombobox.cpp
    appmenuview.cpp
    appchooserdialog.cpp
    filesearchdialog.cpp
    fm-search.c
    trash.cpp
    application.cpp
    filer.cpp
    mainwindow.cpp
    tabpage.cpp
    tabbar.cpp
    view.cpp
    launcher.cpp
    preferencesdialog.cpp
    xdgdir.cpp
    desktoppreferencesdialog.cpp
    desktopmainwindow.cpp
    desktopwindow.cpp
    desktopitemdelegate.cpp
    autorundialog.cpp
    settings.cpp
    metadata.cpp
    windowregistry.cpp
    gotofolderwindow.cpp
    dbusinterface.cpp
)

qt5_add_dbus_adaptor(filer_SRCS
    org.filer.Application.xml
    application.h
    Filer::Application
    applicationadaptor
    ApplicationAdaptor
)

set(filer_UIS
    file-props.ui
    file-operation-dialog.ui
    rename-dialog.ui
    mount-operation-password.ui
    edit-bookmarks.ui
    exec-file.ui
    app-chooser-dialog.ui
    filesearch.ui
    main-win.ui
    about.ui
    preferences.ui
    desktop-preferences.ui
    desktop-folder.ui
    autorun.ui
)

qt5_wrap_ui(filer_UIS_H ${filer_UIS})

# Create TS_FILES variable that contains all .ts files
file(GLOB TS_FILES translations/*.ts)

# Create/update .ts files from the source code
qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
# message(FATAL_ERROR "${QM_FILES}")

# Compile .ts to .qm
qt5_add_translation(TRANSLATION_QM ${TS_FILES})

add_custom_target(translations_update DEPENDS ${QM_FILES})

add_custom_target(translations DEPENDS ${TRANSLATION_QM})

# translate desktop entry files for Filer and desktop preferences
lxqt_translate_desktop(DESKTOP_FILES
    SOURCES
        filer-qt.desktop.in
        filer-qt-desktop-pref.desktop.in
)

# Let CMake handle the rpath setting for us
# https://github.com/conan-io/conan/issues/3996
if(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path") # Darwin, macOS
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib") # FreeBSD
endif()
if(NOT LINUX)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON) # On Linux we let appimagetool handle it
endif()

add_executable(Filer
    ${filer_SRCS}
    ${filer_UIS_H}
    ${QM_FILES}
    ${DESKTOP_FILES}
)
set_property(
     TARGET Filer APPEND
     PROPERTY COMPILE_DEFINITIONS
     LIBFM_QT_API=Q_DECL_IMPORT
     PCMANFM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/filer"
     LIBFM_DATA_DIR="${LIBFM_PREFIX}/share/libfm" # This is a little bit dirty
     FILER_VERSION="${FILER_VERSION}"
)

target_link_libraries(Filer
    ${QTX_LIBRARIES}
    ${LIBFM_LIBRARIES}
    ${LIBMENUCACHE_LIBRARIES}
    ${SYSTEM_LIBS_LIBRARIES}
)

install(TARGETS Filer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# install a desktop entry file for Filer and desktop preferences
install(FILES ${DESKTOP_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

install(FILES ${QM_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/filer/translations")

# prevent the generated files from being deleted during make clean
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)

qt5_use_modules(Filer Widgets DBus)
