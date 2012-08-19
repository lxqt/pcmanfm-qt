project(demo)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

find_package(PkgConfig)
pkg_check_modules(LIBFM
  glib-2.0
  gio-2.0
  gio-unix-2.0
  libfm
)

include_directories(
  ${QT_INCLUDES}
  ${LIBFM_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  qmodeltest
)

link_directories(
  ${LIBFM_LIBRARY_DIRS}
)

set(CMAKE_CXX_FLAGS "-DQT_NO_KEYWORDS -fpermissive")

set(qmodeltest_SRCS
  qmodeltest/modeltest.cpp
  qmodeltest/dynamictreemodel.cpp
  qmodeltest/dynamictreemodel.h
  # qmodeltest/tst_modeltest.cpp
)

set(demo_SRCS
  demo.cpp
  main.cpp
  application.cpp
  icontheme.cpp
  filelauncher.cpp
  proxyfoldermodel.cpp
  foldermodel.cpp
  folderview.cpp
  ${qmodeltest_SRCS}
)

set(demo_UIS
  main-win.ui
)

QT4_WRAP_UI(demo_UIS_H ${demo_UIS})

qt4_automoc(${demo_SRCS})
add_executable(demo 
  ${demo_SRCS}
  ${demo_UIS_H}
)
target_link_libraries(demo 
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTTEST_LIBRARY}
  ${LIBFM_LIBRARIES}
)
