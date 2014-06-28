project(fm-qt)
if(USE_QT5)
	set(LIBRARY_NAME "fm-qt5")
	set(QTX_INCLUDE_DIRS "")
	set(QTX_LIBRARIES Qt5::Widgets Qt5::X11Extras)
else()
	set(LIBRARY_NAME "fm-qt")
	set(QTX_INCLUDE_DIRS ${QT_INCLUDES})
	set(QTX_LIBRARIES ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
endif()

include_directories(
  ${QTX_INCLUDE_DIRS}
  ${LIBFM_INCLUDE_DIRS}
  ${LIBFM_INCLUDEDIR}/libfm # to workaround incorrect #include in fm-actions.
  ${LIBMENUCACHE_INCLUDE_DIRS}
  ${SYSTEM_LIBS_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

link_directories(
  ${LIBFM_LIBRARY_DIRS}
  ${LIBMENUCACHE_LIBRARY_DIRS}
  ${SYSTEM_LIBS_LIBRARY_DIRS}
)

set(libfm_SRCS
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
)

set(libfm_UIS
  file-props.ui
  file-operation-dialog.ui
  rename-dialog.ui
  mount-operation-password.ui
  edit-bookmarks.ui
  exec-file.ui
  app-chooser-dialog.ui
)

if(USE_QT5)
  qt5_wrap_ui(libfm_UIS_H ${libfm_UIS})
else()
  qt4_wrap_ui(libfm_UIS_H ${libfm_UIS})
endif()

add_library(${LIBRARY_NAME} SHARED
  ${libfm_SRCS}
  ${libfm_UIS_H}
)

set_property(
  TARGET ${LIBRARY_NAME} APPEND
  PROPERTY COMPILE_DEFINITIONS
  LIBFM_QT_COMPILATION=1
  LIBFM_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/libfm-qt"
)

# only turn on custom actions support if it is enabled in libfm.
if(EXISTS ${LIBFM_INCLUDEDIR}/libfm/fm-actions.h )
  set_property(TARGET ${LIBRARY_NAME} APPEND PROPERTY COMPILE_DEFINITIONS CUSTOM_ACTIONS)
endif()

target_link_libraries(${LIBRARY_NAME}
  ${QTX_LIBRARIES}
  ${LIBFM_LIBRARIES}
  ${LIBMENUCACHE_LIBRARIES}
  ${SYSTEM_LIBS_LIBRARIES}
)

# set libtool soname
set_target_properties(
  ${LIBRARY_NAME}
  PROPERTIES SOVERSION "0.0.0"
)

# install include header files (FIXME: can we make this cleaner? should dir name be versioned?)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
  PATTERN "*_p.h" EXCLUDE # exclude private headers
)

# FIXME: add libtool version to the lib (soname) later.
# FIXME: only export public symbols

install(TARGETS ${LIBRARY_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER
)

# install a pkgconfig file for libfm-qt
if(USE_QT5)
	set(REQUIRED_QT "Qt5Core >= 5.1 Qt5DBus >= 5.1")
else()
	set(REQUIRED_QT "QtGui >= 4.7 QtDBus >= 4.7")
endif()
configure_file(libfm-qt.pc.in lib${LIBRARY_NAME}.pc @ONLY)
# FreeBSD loves to install files to different locations
# http://www.freebsd.org/doc/handbook/dirstructure.html
if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${LIBRARY_NAME}.pc DESTINATION libdata/pkgconfig)
else()
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${LIBRARY_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

# add translation for pcmanfm-qt
# See http://www.cmake.org/Wiki/CMake:How_To_Build_Qt4_Software
file(GLOB TS_FILES translations/*.ts)
if(USE_QT5)
  qt5_create_translation(QM_FILES ${libfm_SRCS} ${libfm_UIS} ${TS_FILES})
  qt5_add_translation(QM_FILES ${TS_FILES})
else()
  qt4_create_translation(QM_FILES ${libfm_SRCS} ${libfm_UIS} ${TS_FILES})
  qt4_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target (libfm_translations DEPENDS ${QM_FILES})
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/libfm-qt/translations)

# prevent the generated files from being deleted during make cleaner
set_directory_properties(PROPERTIES
  CLEAN_NO_CUSTOM true
)
