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

set(libfm_SRCS
  bookmarkaction.cpp
  sidepane.cpp
  application.cpp
  icontheme.cpp
  filelauncher.cpp
  foldermodel.cpp
  proxyfoldermodel.cpp
  folderview.cpp
  folderitemdelegate.cpp
  filemenu.cpp
  foldermenu.cpp
  filepropsdialog.cpp
  applaunchcontext.cpp
  placesview.cpp
  placesmodel.cpp
  mountoperation.cpp
  mountoperationpassworddialog.cpp
  mountoperationquestiondialog.cpp
  fileoperation.cpp
  fileoperationdialog.cpp
  renamedialog.cpp
  pathcompleter.cpp
  colorbutton.cpp
  fontbutton.cpp
  browsehistory.cpp
  utilities.cpp
  dndactionmenu.cpp
)

set(libfm_UIS
  file-props.ui
  file-operation-dialog.ui
  rename-dialog.ui
  mount-operation-password.ui
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
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION include
  FILES_MATCHING PATTERN "*.h")

# FIXME: add libtool version to the lib (soname) later.
# FIXME: only export public symbols

install(TARGETS fm-qt
  LIBRARY DESTINATION lib
  PUBLIC_HEADER 
)

# install a pkgconfig file for libfm-qt
configure_file(libfm-qt.pc.in libfm-qt.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libfm-qt.pc
  DESTINATION lib/pkgconfig
)
