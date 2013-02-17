project(pcmanfm-qt)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

find_package(PkgConfig)
pkg_check_modules(LIBFM
  glib-2.0
  gio-2.0
  gio-unix-2.0
  libfm
  libstartup-notification-1.0
)

include_directories(
  ${QT_INCLUDES}
  ${LIBFM_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  qmodeltest
)

link_directories(
  ${LIBFM_LIBRARY_DIRS}
)

set(CMAKE_CXX_FLAGS "-DQT_NO_KEYWORDS -fpermissive")

#set(qmodeltest_SRCS
#  qmodeltest/modeltest.cpp
#  qmodeltest/dynamictreemodel.cpp
#  qmodeltest/dynamictreemodel.h
  # qmodeltest/tst_modeltest.cpp
#)

set(pcmanfm_SRCS
  pcmanfm.cpp
  mainwindow.cpp
  sidepane.cpp
  tabpage.cpp
  application.cpp
  icontheme.cpp
  filelauncher.cpp
  foldermodel.cpp
  proxyfoldermodel.cpp
  folderview.cpp
  filemenu.cpp
  filepropsdialog.cpp
  applaunchcontext.cpp
  placesview.cpp
  placesmodel.cpp
  mountoperation.cpp
#  ${qmodeltest_SRCS}
)

set(pcmanfm_UIS
  main-win.ui
  file-props.ui
  mount-operation-password.ui
  about.ui
)

QT4_WRAP_UI(pcmanfm_UIS_H ${pcmanfm_UIS})

qt4_automoc(${pcmanfm_SRCS})
add_executable(pcmanfm-qt 
  ${pcmanfm_SRCS}
  ${pcmanfm_UIS_H}
)
target_link_libraries(pcmanfm-qt 
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTTEST_LIBRARY}
  ${LIBFM_LIBRARIES}
)
