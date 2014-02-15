include_directories(
  ${QT_INCLUDES}
  ${LIBFM_INCLUDE_DIRS}
  ${LIBFM_libfm_INCLUDEDIR}/libfm # to workaround incorrect #include in fm-actions.
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/libfm-qt
)

link_directories(
  ${LIBFM_LIBRARY_DIRS}
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
)

set(libfm_UIS
  file-props.ui
  file-operation-dialog.ui
  rename-dialog.ui
  mount-operation-password.ui
  edit-bookmarks.ui
)

QT4_WRAP_UI(libfm_UIS_H ${libfm_UIS})

add_library(fm-qt SHARED
  ${libfm_SRCS}
  ${libfm_UIS_H}
)

set_property(
  TARGET fm-qt APPEND
  PROPERTY COMPILE_DEFINITIONS
  LIBFM_QT_COMPILATION=1
  LIBFM_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/libfm-qt"
)

target_link_libraries(fm-qt
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${LIBFM_LIBRARIES}
)

# set libtool soname
set_target_properties(
  fm-qt
  PROPERTIES SOVERSION "0.0.0"
)

# install include header files (FIXME: can we make this cleaner? should dir name be versioned?)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h")

# FIXME: add libtool version to the lib (soname) later.
# FIXME: only export public symbols

install(TARGETS fm-qt
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER 
)

# install a pkgconfig file for libfm-qt
configure_file(libfm-qt.pc.in libfm-qt.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libfm-qt.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# add translation for pcmanfm-qt
# See http://www.cmake.org/Wiki/CMake:How_To_Build_Qt4_Software
file(GLOB TS_FILES translations/*.ts)
qt4_create_translation(QM_FILES
  ${libfm_SRCS}
  ${libfm_UIS}
  ${TS_FILES})
qt4_add_translation(QM_FILES ${TS_FILES})
add_custom_target (libfm_translations DEPENDS ${QM_FILES})
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/libfm-qt/translations)

# prevent the generated files from being deleted during make cleaner
set_directory_properties(PROPERTIES
  CLEAN_NO_CUSTOM true
)
