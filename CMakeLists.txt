cmake_minimum_required(VERSION 3.0.2)
project(pcmanfm-qt)

set(PCMANFM_QT_VERSION_MAJOR 0)
set(PCMANFM_QT_VERSION_MINOR 9)
set(PCMANFM_QT_VERSION_PATCH 0)
set(PCMANFM_QT_VERSION ${PCMANFM_QT_VERSION_MAJOR}.${PCMANFM_QT_VERSION_MINOR}.${PCMANFM_QT_VERSION_PATCH})

set(LIBFM_QT_VERSION_MAJOR 0)
set(LIBFM_QT_VERSION_MINOR 9)
set(LIBFM_QT_VERSION_PATCH 0)
set(LIBFM_QT_VERSION ${LIBFM_QT_VERSION_MAJOR}.${LIBFM_QT_VERSION_MINOR}.${LIBFM_QT_VERSION_PATCH})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# We use the libtool versioning scheme for the internal so name, "current:revision:age"
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info
# https://www.sourceware.org/autobook/autobook/autobook_91.html
# http://pusling.com/blog/?p=352
# Actually, libtool uses different ways on different operating systems. So there is no
# universal way to translate a libtool version-info to a cmake version.
# We use "(current-age).age.revision" as the cmake version.
# current: 2, revision: 0, age: 0 => version: 2.0.0
set(LIBFM_QT_LIB_VERSION "2.0.0")
set(LIBFM_QT_LIB_SOVERSION "2")

find_package(Qt5Widgets 5.2 REQUIRED)
find_package(Qt5DBus 5.2 REQUIRED)
find_package(Qt5LinguistTools 5.2 REQUIRED)
find_package(Qt5X11Extras 5.2 REQUIRED)

find_package(PkgConfig)
pkg_check_modules(SYSTEM_LIBS REQUIRED
    glib-2.0
    gio-2.0
    gio-unix-2.0
    xcb
)

pkg_check_modules(LIBFM REQUIRED libfm>=1.2.0)
pkg_check_modules(LIBMENUCACHE REQUIRED libmenu-cache>=0.4.0)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
include(GNUInstallDirs)
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)

add_definitions(-DQT_NO_KEYWORDS)

if (CMAKE_COMPILER_IS_GNUCXX)
    # set visibility to hidden to hide symbols, unless they're exported manually in the code
    set(CMAKE_CXX_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden -fno-exceptions ${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(libfm-qt)
add_subdirectory(pcmanfm)

# manpage for pcmanfm-qt
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/pcmanfm-qt.1.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt.1"
    @ONLY
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
)

# add Doxygen support to generate API docs
# References:
# http://majewsky.wordpress.com/2010/08/14/tip-of-the-day-cmake-and-doxygen/
# http://www.bluequartz.net/projects/EIM_Segmentation/SoftwareDocumentation/html/usewithcmakeproject.html
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)
if(BUILD_DOCUMENTATION)
    find_package(Doxygen REQUIRED)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(doc ALL
        ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs" DESTINATION "${CMAKE_INSTALL_DOCDIR}")
endif()

# building tarball with CPack -------------------------------------------------
# To create a source distribution, type:
# make package_source
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VENDOR "")
set(CPACK_PACKAGE_VERSION_MAJOR ${PCMANFM_QT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PCMANFM_QT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PCMANFM_QT_VERSION_PATCH})
set(CPACK_GENERATOR TBZ2)
set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include(CPack)
