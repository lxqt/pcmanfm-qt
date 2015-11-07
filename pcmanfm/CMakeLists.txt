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

set(pcmanfm_SRCS
    application.cpp
    pcmanfm.cpp
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

qt5_add_dbus_adaptor(pcmanfm_SRCS
    org.pcmanfm.Application.xml
    application.h
    PCManFM::Application
    applicationadaptor
    ApplicationAdaptor
)

set(pcmanfm_UIS
    main-win.ui
    about.ui
    preferences.ui
    desktop-preferences.ui
    desktop-folder.ui
    autorun.ui
)

qt5_wrap_ui(pcmanfm_UIS_H ${pcmanfm_UIS})

# add translation for pcmanfm-qt
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES ${pcmanfm_SRCS} ${pcmanfm_UIS_H}
)

# translate desktop entry files for pcmanfm-qt and desktop preferences
lxqt_translate_desktop(DESKTOP_FILES
    SOURCES
        pcmanfm-qt.desktop.in
        pcmanfm-qt-desktop-pref.desktop.in
)

add_executable(pcmanfm-qt
    ${pcmanfm_SRCS}
    ${pcmanfm_UIS_H}
    ${QM_FILES}
    ${DESKTOP_FILES}
)
set_property(
     TARGET pcmanfm-qt APPEND
     PROPERTY COMPILE_DEFINITIONS
     LIBFM_QT_API=Q_DECL_IMPORT
     PCMANFM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/pcmanfm-qt"
     LIBFM_DATA_DIR="${LIBFM_PREFIX}/share/libfm" # This is a little bit dirty
     PCMANFM_QT_VERSION="${PCMANFM_QT_VERSION}"
)

target_link_libraries(pcmanfm-qt
    ${QTX_LIBRARIES}
    ${LIBFM_LIBRARIES}
    ${LIBMENUCACHE_LIBRARIES}
    ${SYSTEM_LIBS_LIBRARIES}
    ${LIBFM_LIBRARY}
)

install(TARGETS pcmanfm-qt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# install a desktop entry file for pcmanfm-qt and desktop preferences
install(FILES ${DESKTOP_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

install(FILES ${QM_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/pcmanfm-qt/translations")

# prevent the generated files from being deleted during make clean
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)

qt5_use_modules(pcmanfm-qt Widgets DBus)
