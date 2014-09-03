# version >= 2.8.6 is needed because of CMAKE_AUTOMOC feature
cmake_minimum_required(VERSION 2.8.6)
project(pcmanfm-qt)

# Setup common compiler flags/variables ---------------------------------------
set(PCMANFM_QT_VERSION "0.8.0")
set(LIBFM_QT_VERSION "0.8.0")
# We use the libtool versioning scheme for the internal so name, "current:revision:age"
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info
# https://www.sourceware.org/autobook/autobook/autobook_91.html
# http://pusling.com/blog/?p=352
# Actually, libtool uses differnt ways on different operating systems. So there is no
# universal way to translate a libtool version-info to a cmake version.
# We use "(current-age).age.revision" as the cmake version.
# current: 1, revision: 0, age: 0 => version: 1.0.0
set(LIBFM_QT_LIB_VERSION "1.0.0")
set(LIBFM_QT_LIB_SOVERSION "1")

# Set default installation paths
set(LIB_INSTALL_DIR "lib${LIB_SUFFIX}" CACHE PATH "Installation path for libraries")
set(INCLUDE_INSTALL_DIR include CACHE PATH "Installation path for includes")

option(USE_QT5 "Use Qt5. Defaults to Qt4" $ENV{USE_QT5})

if(USE_QT5) # actually, we need Qt >= 5.1. Qt 5.0 is not supported.
  cmake_minimum_required(VERSION 2.8.11)
  find_package(Qt5Widgets 5.1 REQUIRED)
  find_package(Qt5DBus 5.1 REQUIRED)
  find_package(Qt5LinguistTools 5.1 REQUIRED)
  find_package(Qt5X11Extras 5.1 REQUIRED)
  set(XLIB "xcb")
else()
  find_package(Qt4 4.6 REQUIRED QtCore QtGui QtDBus)
  include(${QT_USE_FILE})
  set(XLIB "x11")
endif()

find_package(PkgConfig)
pkg_check_modules(SYSTEM_LIBS REQUIRED
  glib-2.0
  gio-2.0
  gio-unix-2.0
  ${XLIB}
#  libstartup-notification-1.0
)

pkg_check_modules(LIBFM REQUIRED
  libfm>=1.2.0
)

pkg_check_modules(LIBMENUCACHE REQUIRED
  libmenu-cache>=0.4.0
)

include(GNUInstallDirs)

# do not use Qt keywords
add_definitions(
  -DQT_NO_KEYWORDS
)

if (CMAKE_COMPILER_IS_GNUCXX)
  # set visibility to hidden to hide symbols, unlesss they're exporeted manually in the code
  set(CMAKE_CXX_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden -fno-exceptions")
endif()

# Eanble CMake auto-moc support for Qt
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# The core library libfm-qt ---------------------------------------------------
add_subdirectory(libfm-qt)

# The main program pcmanfm ----------------------------------------------------
add_subdirectory(pcmanfm)

# update translations
add_custom_target(update_translations ALL DEPENDS
  libfm_translations
  pcmanfm_translations
)

# manpage for pcmanfm-qt
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pcmanfm-qt.1.in
  ${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt.1
  @ONLY
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt.1
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)

# add Doxygen support to genrate API docs
# References:
# http://majewsky.wordpress.com/2010/08/14/tip-of-the-day-cmake-and-doxygen/
# http://www.bluequartz.net/projects/EIM_Segmentation/SoftwareDocumentation/html/usewithcmakeproject.html
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)
if(BUILD_DOCUMENTATION)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc ALL
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
    
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
      DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )

  endif(DOXYGEN_FOUND)
endif(BUILD_DOCUMENTATION)

# building tarball with CPack -------------------------------------------------
# To create a source distribution, type:
# make package_source
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set (CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
set (CPACK_PACKAGE_VENDOR "")
set (CPACK_PACKAGE_VERSION_MAJOR "0")
set (CPACK_PACKAGE_VERSION_MINOR "8")
set (CPACK_PACKAGE_VERSION_PATCH "0")
set (CPACK_GENERATOR TBZ2)
set (CPACK_SOURCE_GENERATOR TBZ2)
set (CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include (CPack)
