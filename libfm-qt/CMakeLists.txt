project(fm-qt)

set(LIBRARY_NAME "fm-qt5")
set(QTX_INCLUDE_DIRS "")
set(QTX_LIBRARIES Qt5::Widgets Qt5::X11Extras)
set(libfm_TRANSLATION_TEMPLATE "lib${PROJECT_NAME}")

include_directories(
    ${QTX_INCLUDE_DIRS}
    ${LIBFM_INCLUDE_DIRS}
    "${LIBFM_INCLUDEDIR}/libfm" # to workaround incorrect #include in fm-actions.
    ${LIBMENUCACHE_INCLUDE_DIRS}
    ${SYSTEM_LIBS_INCLUDE_DIRS}
    "${CMAKE_CURRENT_BINARY_DIR}"
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
    createnewmenu.cpp
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
    filesearchdialog.cpp
    fm-search.c # might be moved to libfm later
)

set(libfm_UIS
    file-props.ui
    file-operation-dialog.ui
    rename-dialog.ui
    mount-operation-password.ui
    edit-bookmarks.ui
    exec-file.ui
    app-chooser-dialog.ui
    filesearch.ui
)

qt5_wrap_ui(libfm_UIS_H ${libfm_UIS})

# add translation for libfm-qt
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES ${libfm_SRCS} ${libfm_UIS}
    TEMPLATE ${libfm_TRANSLATION_TEMPLATE}
)

add_library(${LIBRARY_NAME} SHARED
    ${libfm_SRCS}
    ${libfm_UIS_H}
    ${QM_FILES}
)

set_property(
    TARGET ${LIBRARY_NAME} APPEND
    PROPERTY COMPILE_DEFINITIONS
    LIBFM_QT_COMPILATION=1
    LIBFM_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/libfm-qt"
)

# only turn on custom actions support if it is enabled in libfm.
if(EXISTS ${LIBFM_INCLUDEDIR}/libfm/fm-actions.h)
    set_property(TARGET ${LIBRARY_NAME} APPEND PROPERTY COMPILE_DEFINITIONS CUSTOM_ACTIONS)
endif()

target_link_libraries(${LIBRARY_NAME}
    ${QTX_LIBRARIES}
    ${LIBFM_LIBRARIES}
    ${LIBMENUCACHE_LIBRARIES}
    ${SYSTEM_LIBS_LIBRARIES}
)

# set libtool soname
set_target_properties(${LIBRARY_NAME} PROPERTIES
    VERSION ${LIBFM_QT_LIB_VERSION}
    SOVERSION ${LIBFM_QT_LIB_SOVERSION}
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
set(REQUIRED_QT "Qt5Core >= 5.1 Qt5DBus >= 5.1")
configure_file(libfm-qt.pc.in lib${LIBRARY_NAME}.pc @ONLY)
# FreeBSD loves to install files to different locations
# http://www.freebsd.org/doc/handbook/dirstructure.html
if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lib${LIBRARY_NAME}.pc" DESTINATION libdata/pkgconfig)
else()
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lib${LIBRARY_NAME}.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

install(FILES ${QM_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/libfm-qt/translations")

# prevent the generated files from being deleted during make cleaner
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)
