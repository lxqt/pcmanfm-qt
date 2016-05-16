cmake_minimum_required(VERSION 3.0.2)
project(pcmanfm-qt)

# CMP0063: Honor visibility properties for all target types.
if (POLICY CMP0063)
    cmake_policy (SET CMP0063 NEW)
endif (POLICY CMP0063)

set(PCMANFM_QT_VERSION_MAJOR 0)
set(PCMANFM_QT_VERSION_MINOR 11)
set(PCMANFM_QT_VERSION_PATCH 0)
set(PCMANFM_QT_VERSION ${PCMANFM_QT_VERSION_MAJOR}.${PCMANFM_QT_VERSION_MINOR}.${PCMANFM_QT_VERSION_PATCH})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Qt5Widgets 5.2 REQUIRED)
find_package(Qt5DBus 5.2 REQUIRED)
find_package(Qt5LinguistTools 5.2 REQUIRED)
find_package(Qt5X11Extras 5.2 REQUIRED)
find_package(fm-qt REQUIRED)
#Note: no run-time dependency on liblxqt, just a build dependency for lxqt_translate_ts/desktop
find_package(lxqt REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIB_XCB REQUIRED xcb)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
include(GNUInstallDirs)
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)

# set visibility to hidden to hide symbols, unless they're exported manually in the code
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if (CMAKE_VERSION VERSION_LESS "3.1")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
        # -std=c++0x is deprecated but some tools e.g. qmake or older gcc are still using it
        if(COMPILER_SUPPORTS_CXX0X)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        else()
            message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} does not support c++11/c++0x")
        endif()
    endif()
else()
  set(CMAKE_CXX_STANDARD 11)
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # set visibility to hidden to hide symbols, unless they're exported manually in the code
    set(CMAKE_CXX_FLAGS "-fno-exceptions ${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

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
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VENDOR "")
set(CPACK_PACKAGE_VERSION_MAJOR ${PCMANFM_QT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PCMANFM_QT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PCMANFM_QT_VERSION_PATCH})
set(CPACK_GENERATOR TBZ2)
set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include(CPack)
