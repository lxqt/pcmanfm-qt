include_directories(
  ${QT_INCLUDES}
  ${LIBFM_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/libfm-qt
)

link_directories(
  ${LIBFM_LIBRARY_DIRS}
)

set(pcmanfm_SRCS
  application.cpp
  pcmanfm.cpp
  mainwindow.cpp
  tabpage.cpp
  view.cpp
  preferencesdialog.cpp
  desktoppreferencesdialog.cpp
  desktopwindow.cpp
  desktopitemdelegate.cpp
  settings.cpp
)

qt4_add_dbus_adaptor(pcmanfm_SRCS
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
)

QT4_WRAP_UI(pcmanfm_UIS_H ${pcmanfm_UIS})
qt4_automoc(${pcmanfm_SRCS})

add_executable(pcmanfm-qt 
  ${pcmanfm_SRCS}
  ${pcmanfm_UIS_H}
)
set_property(
   TARGET pcmanfm-qt APPEND
   PROPERTY COMPILE_DEFINITIONS
   LIBFM_QT_API=Q_DECL_IMPORT
)
target_link_libraries(pcmanfm-qt 
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${LIBFM_LIBRARIES}
  fm-qt
)

install(TARGETS
  pcmanfm-qt
  RUNTIME DESTINATION bin
)
# install a desktop entry file for pcmanfm-qt and desktop preferences
configure_file(pcmanfm-qt.desktop.in pcmanfm-qt.desktop @ONLY)
configure_file(pcmanfm-qt-desktop-pref.desktop.in pcmanfm-qt-desktop-pref.desktop @ONLY)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt.desktop
        ${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt-desktop-pref.desktop
  DESTINATION share/applications
)

# add translation for pcmanfm-qt
# See http://www.cmake.org/Wiki/CMake:How_To_Build_Qt4_Software
file(GLOB TS_FILES translations/*.ts)
if(UPDATE_TRANSLATIONS)
  qt4_create_translation(QM_FILES ${FILES_TO_TRANSLATE} ${TRANSLATIONS_FILES})
else (UPDATE_TRANSLATIONS)
  qt4_add_translation(QM_FILES ${TRANSLATIONS_FILES})
endif(UPDATE_TRANSLATIONS) 
add_custom_target (translations_target DEPENDS ${QM_FILES})
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/translations)
