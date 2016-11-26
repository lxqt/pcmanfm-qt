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
    connectserverdialog.cpp
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
    connect.ui
)

qt5_wrap_ui(pcmanfm_UIS_H ${pcmanfm_UIS})

# add translation for pcmanfm-qt
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES ${pcmanfm_SRCS} ${pcmanfm_UIS}
    INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations"
    PULL_TRANSLATIONS ${PULL_TRANSLATIONS}
    CLEAN_TRANSLATIONS ${CLEAN_TRANSLATIONS}
    TRANSLATIONS_REPO ${TRANSLATIONS_REPO}
    TRANSLATIONS_REFSPEC ${TRANSLATIONS_REFSPEC}
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

target_compile_definitions(pcmanfm-qt
    PRIVATE
        PCMANFM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/pcmanfm-qt"
        PCMANFM_QT_VERSION="${PCMANFM_QT_VERSION}"
        LIBFM_DATA_DIR="${PKG_FM_PREFIX}/share/libfm"
)

target_include_directories(pcmanfm-qt
    PRIVATE "${LIB_XCB_INDLUDE_DIRS}"
    "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
)

target_link_libraries(pcmanfm-qt
    Qt5::X11Extras
    Qt5::Widgets
    Qt5::DBus
    fm-qt
    "${LIB_XCB_LIBRARIES}"
)

install(TARGETS pcmanfm-qt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# install a desktop entry file for pcmanfm-qt and desktop preferences
install(FILES ${DESKTOP_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

# prevent the generated files from being deleted during make clean
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)
