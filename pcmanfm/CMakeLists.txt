if(USE_QT5)
	set(LIBFM_LIBRARY "fm-qt5")
	set(QTX_INCLUDE_DIRS "")
	set(QTX_LIBRARIES Qt5::Widgets Qt5::DBus Qt5::X11Extras)
else()
	set(LIBFM_LIBRARY "fm-qt")
	set(QTX_INCLUDE_DIRS ${QT_INCLUDES})
	set(QTX_LIBRARIES ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTDBUS_LIBRARY})
endif()

include_directories(
  ${QTX_INCLUDE_DIRS}
  ${LIBFM_INCLUDE_DIRS}
  ${LIBMENUCACHE_INCLUDE_DIRS}
  ${SYSTEM_LIBS_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/libfm-qt
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
  desktoppreferencesdialog.cpp
  desktopwindow.cpp
  desktopitemdelegate.cpp
  autorundialog.cpp
  settings.cpp
)

if(USE_QT5)
  qt5_add_dbus_adaptor(pcmanfm_SRCS
    org.pcmanfm.Application.xml
    application.h
    PCManFM::Application
    applicationadaptor
    ApplicationAdaptor
  )
else()
  qt4_add_dbus_adaptor(pcmanfm_SRCS
    org.pcmanfm.Application.xml
    application.h
    PCManFM::Application
    applicationadaptor
    ApplicationAdaptor
  )
endif()

set(pcmanfm_UIS
  main-win.ui
  about.ui
  preferences.ui
  desktop-preferences.ui
  autorun.ui
)

if(USE_QT5)
  qt5_wrap_ui(pcmanfm_UIS_H ${pcmanfm_UIS})
else()
  qt4_wrap_ui(pcmanfm_UIS_H ${pcmanfm_UIS})
endif()

add_executable(pcmanfm-qt 
  ${pcmanfm_SRCS}
  ${pcmanfm_UIS_H}
)
set_property(
   TARGET pcmanfm-qt APPEND
   PROPERTY COMPILE_DEFINITIONS
   LIBFM_QT_API=Q_DECL_IMPORT
   PCMANFM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/pcmanfm-qt"
   LIBFM_DATA_DIR="${LIBFM_PREFIX}/share/libfm" # This is a little bit dirty
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
configure_file(pcmanfm-qt.desktop.in pcmanfm-qt.desktop @ONLY)
configure_file(pcmanfm-qt-desktop-pref.desktop.in pcmanfm-qt-desktop-pref.desktop @ONLY)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt.desktop
        ${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt-desktop-pref.desktop
  DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

# add translation for pcmanfm-qt
# See http://www.cmake.org/Wiki/CMake:How_To_Build_Qt4_Software
file(GLOB TS_FILES translations/*.ts)

if(USE_QT5)
  qt5_create_translation(QM_FILES ${pcmanfm_SRCS} ${pcmanfm_UIS_H} ${TS_FILES})
  qt5_add_translation(QM_FILES ${TS_FILES})
else()
  qt4_create_translation(QM_FILES ${pcmanfm_SRCS} ${pcmanfm_UIS_H} ${TS_FILES})
  qt4_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target (pcmanfm_translations DEPENDS ${QM_FILES})
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/pcmanfm-qt/translations)

# prevent the generated files from being deleted during make clean
set_directory_properties(PROPERTIES
  CLEAN_NO_CUSTOM true
)

if(USE_QT5)
  qt5_use_modules(pcmanfm-qt Widgets DBus)
endif()
