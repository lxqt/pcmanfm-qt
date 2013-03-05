project(pcmanfm-qt)
cmake_minimum_required(VERSION 2.6)

find_package(Qt4 4.7 REQUIRED  QtCore QtGui QtDBus)
include(${QT_USE_FILE})

find_package(PkgConfig)
pkg_check_modules(LIBFM REQUIRED
  glib-2.0
  gio-2.0
  gio-unix-2.0
  libfm>=1.1.0
#  libstartup-notification-1.0
  x11
)

# find_package(Boost REQUIRED)

include_directories(
  ${QT_INCLUDES}
  ${LIBFM_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/libfm-qt
  # ${Boost_INCLUDE_DIRS}
  # qmodeltest
)

link_directories(
  ${LIBFM_LIBRARY_DIRS}
)

set(CMAKE_CXX_FLAGS "-DQT_NO_KEYWORDS -fpermissive -fvisibility=hidden -fno-exceptions")

#set(qmodeltest_SRCS
#  qmodeltest/modeltest.cpp
#  qmodeltest/dynamictreemodel.cpp
#  qmodeltest/dynamictreemodel.h
  # qmodeltest/tst_modeltest.cpp
#)

set(libfm_SRCS
  libfm-qt/bookmarkaction.cpp
  libfm-qt/sidepane.cpp
  libfm-qt/application.cpp
  libfm-qt/icontheme.cpp
  libfm-qt/filelauncher.cpp
  libfm-qt/foldermodel.cpp
  libfm-qt/proxyfoldermodel.cpp
  libfm-qt/folderview.cpp
  libfm-qt/folderitemdelegate.cpp
  libfm-qt/filemenu.cpp
  libfm-qt/foldermenu.cpp
  libfm-qt/filepropsdialog.cpp
  libfm-qt/applaunchcontext.cpp
  libfm-qt/placesview.cpp
  libfm-qt/placesmodel.cpp
  libfm-qt/mountoperation.cpp
  libfm-qt/mountoperationpassworddialog.cpp
  libfm-qt/mountoperationquestiondialog.cpp
  libfm-qt/fileoperation.cpp
  libfm-qt/fileoperationdialog.cpp
  libfm-qt/renamedialog.cpp
  libfm-qt/pathcompleter.cpp
  libfm-qt/colorbutton.cpp
  libfm-qt/fontbutton.cpp
  libfm-qt/browsehistory.cpp
  libfm-qt/utilities.cpp
)

set(libfm_UIS
  libfm-qt/file-props.ui
  libfm-qt/file-operation-dialog.ui
  libfm-qt/rename-dialog.ui
  libfm-qt/mount-operation-password.ui
)

QT4_WRAP_UI(libfm_UIS_H ${libfm_UIS})
qt4_automoc(${libfm_SRCS})

add_library(fm-qt SHARED
  ${libfm_SRCS}
  ${libfm_UIS_H}
)

set_property(
   TARGET fm-qt APPEND
   PROPERTY COMPILE_DEFINITIONS
   LIBFM_QT_API=Q_DECL_EXPORT
)

target_link_libraries(fm-qt
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${LIBFM_LIBRARIES}
)

# install include header files (FIXME: can we make this cleaner? should dir name be versioned?)
install(DIRECTORY libfm-qt/ DESTINATION include/libfm-qt
          FILES_MATCHING PATTERN "*.h")

# FIXME: add libtool version to the lib (soname) later.
# FIXME: only export public symbols

install(TARGETS fm-qt
  LIBRARY DESTINATION lib
  PUBLIC_HEADER 
)


set(pcmanfm_SRCS
  pcmanfm/application.cpp
  pcmanfm/pcmanfm.cpp
  pcmanfm/mainwindow.cpp
  pcmanfm/tabpage.cpp
  pcmanfm/view.cpp
  pcmanfm/preferencesdialog.cpp
  pcmanfm/desktoppreferencesdialog.cpp
  pcmanfm/desktopwindow.cpp
  pcmanfm/desktopitemdelegate.cpp
  pcmanfm/settings.cpp
)

qt4_add_dbus_adaptor(pcmanfm_SRCS
  pcmanfm/org.pcmanfm.Application.xml
  pcmanfm/application.h
  PCManFM::Application
  applicationadaptor
  ApplicationAdaptor
)

set(pcmanfm_UIS
  pcmanfm/main-win.ui
  pcmanfm/about.ui
  pcmanfm/preferences.ui
  pcmanfm/desktop-preferences.ui
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

# building tarball with CPack ------------------------------------------
# To create a source distribution, type:
# make package_source
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set (CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
set (CPACK_PACKAGE_VENDOR "")
set (CPACK_PACKAGE_VERSION_MAJOR "0")
set (CPACK_PACKAGE_VERSION_MINOR "1")
set (CPACK_PACKAGE_VERSION_PATCH "0")
set (CPACK_GENERATOR TBZ2)
set (CPACK_SOURCE_GENERATOR TBZ2)
set (CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
# set (CPACK_SOURCE_PACKAGE_FILE_NAME pcmanfm-qt.0.1.0.tar.gz)
include (CPack)
