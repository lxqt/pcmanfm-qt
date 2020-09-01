set(LIBFM_LIBRARY "fm-qt5")
set(QTX_INCLUDE_DIRS "")
set(QTX_LIBRARIES Qt5::Widgets Qt5::DBus Qt5::X11Extras)

include_directories(
    ${QTX_INCLUDE_DIRS}
    ${LIBFM_INCLUDE_DIRS}
    ${LIBMENUCACHE_INCLUDE_DIRS}
    ${SYSTEM_LIBS_INCLUDE_DIRS}
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/libfm-qt"
)

link_directories(
    ${LIBFM_LIBRARY_DIRS}
    ${LIBMENUCACHE_LIBRARY_DIRS}
    ${SYSTEM_LIBS_LIBRARY_DIRS}
)

set(filer_SRCS
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
    desktopwindow.cpp
    desktopitemdelegate.cpp
    autorundialog.cpp
    settings.cpp
)

qt5_add_dbus_adaptor(filer_SRCS
    org.filer.Application.xml
    application.h
    Filer::Application
    applicationadaptor
    ApplicationAdaptor
)

set(filer_UIS
    main-win.ui
    about.ui
    preferences.ui
    desktop-preferences.ui
    desktop-folder.ui
    autorun.ui
)

qt5_wrap_ui(filer_UIS_H ${filer_UIS})

# add translation for filer-qt
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES ${filer_SRCS} ${filer_UIS_H}
)

# translate desktop entry files for filer-qt and desktop preferences
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

add_executable(filer-qt
    ${filer_SRCS}
    ${filer_UIS_H}
    ${QM_FILES}
    ${DESKTOP_FILES}
)
set_property(
     TARGET filer-qt APPEND
     PROPERTY COMPILE_DEFINITIONS
     LIBFM_QT_API=Q_DECL_IMPORT
     PCMANFM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/filer-qt"
     LIBFM_DATA_DIR="${LIBFM_PREFIX}/share/libfm" # This is a little bit dirty
     PCMANFM_QT_VERSION="${PCMANFM_QT_VERSION}"
)

target_link_libraries(filer-qt
    ${QTX_LIBRARIES}
    ${LIBFM_LIBRARIES}
    ${LIBMENUCACHE_LIBRARIES}
    ${SYSTEM_LIBS_LIBRARIES}
    ${LIBFM_LIBRARY}
)

install(TARGETS filer-qt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# install a desktop entry file for filer-qt and desktop preferences
install(FILES ${DESKTOP_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

install(FILES ${QM_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/filer-qt/translations")

# prevent the generated files from being deleted during make clean
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)

qt5_use_modules(filer-qt Widgets DBus)
