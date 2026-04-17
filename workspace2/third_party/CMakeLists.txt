# =============================================================================
# third_party/CMakeLists.txt
# =============================================================================
#
# USAGE
#
# 1. Build once:
#
# rm -rf build/third_party
# cmake -S third_party -B build/third_party -G Ninja -DCMAKE_INSTALL_PREFIX=$PWD/build/third_party/install
# cmake --build build/third_party --target install
#
# 2. Use in main project:
#
# list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/build/third_party/install")
# find_package(third_party REQUIRED)
# target_link_libraries(my_tests PRIVATE
#     third_party::gtest
#     third_party::gtest_main
# )

cmake_minimum_required(VERSION 3.25)
project(third_party LANGUAGES CXX)

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

cmake_policy(SET CMP0135 NEW)

# ------------------------------------------------------------------------------
# Global configuration
# ------------------------------------------------------------------------------

# Source extraction goes to the source tree (committed to git).
# FETCHCONTENT_BASE_DIR controls where FetchContent puts <name>-src.
# BINARY_DIR and SUBBUILD_DIR are overridden per-dependency (see add_third_party)
# so that build artifacts stay in CMAKE_BINARY_DIR, not the source tree.
set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(FETCHCONTENT_QUIET OFF)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR})
set(DOWNLOAD_DIR ${THIRD_PARTY_DIR}/downloads)

file(MAKE_DIRECTORY ${DOWNLOAD_DIR})

# Build static libs only (recommended)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ------------------------------------------------------------------------------
# Helper: download archive if missing
# ------------------------------------------------------------------------------
function(download_if_missing name archive_path url)
    if (EXISTS "${archive_path}")
        message(STATUS "${name}: using cached archive")
        return()
    endif()

    message(STATUS "${name}: downloading ${url}")
    file(DOWNLOAD
        "${url}"
        "${archive_path}"
        SHOW_PROGRESS
        TLS_VERIFY ON
        STATUS status
    )

    list(GET status 0 code)
    if (NOT code EQUAL 0)
        list(GET status 1 msg)
        message(FATAL_ERROR "${name}: download failed: ${msg}")
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Helper: declare dependency (local source OR downloaded archive)
# ------------------------------------------------------------------------------
function(add_third_party name)
    set(options)
    set(oneValueArgs VERSION URL)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "" ${ARGN})

    set(SRC_DIR ${FETCHCONTENT_BASE_DIR}/${name}-src)

    if (EXISTS ${SRC_DIR})
        message(STATUS "${name}: using local source ${SRC_DIR}")

        FetchContent_Declare(${name}
            SOURCE_DIR   ${SRC_DIR}
            BINARY_DIR   ${CMAKE_BINARY_DIR}/${name}-build
            SUBBUILD_DIR ${CMAKE_BINARY_DIR}/${name}-subbuild
        )
    else()
        if (NOT ARG_VERSION OR NOT ARG_URL)
            message(FATAL_ERROR "${name}: VERSION and URL are required")
        endif()

        set(ARCHIVE ${name}-${ARG_VERSION}.tar.gz)
        set(ARCHIVE_PATH ${DOWNLOAD_DIR}/${ARCHIVE})

        download_if_missing(${name} ${ARCHIVE_PATH} ${ARG_URL})

        FetchContent_Declare(${name}
            URL          file://${ARCHIVE_PATH}
            BINARY_DIR   ${CMAKE_BINARY_DIR}/${name}-build
            SUBBUILD_DIR ${CMAKE_BINARY_DIR}/${name}-subbuild
        )
    endif()

    FetchContent_MakeAvailable(${name})
endfunction()

# =============================================================================
# Dependencies
# =============================================================================

# ------------------------------------------------------------------------------
# GoogleTest
# ------------------------------------------------------------------------------
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)

add_third_party(googletest
    VERSION 1.12.1
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
)

# Fix include paths for install usage
target_include_directories(gtest PUBLIC
    $<BUILD_INTERFACE:${FETCHCONTENT_BASE_DIR}/googletest-src/googletest/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_include_directories(gtest_main PUBLIC
    $<BUILD_INTERFACE:${FETCHCONTENT_BASE_DIR}/googletest-src/googletest/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Create namespace aliases
add_library(third_party::gtest ALIAS gtest)
add_library(third_party::gtest_main ALIAS gtest_main)

# =============================================================================
# Install & Export
# =============================================================================

# ---- Install libraries ----
install(TARGETS
    gtest
    gtest_main
    gmock
    gmock_main
    EXPORT third_partyTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- Install headers ----
install(DIRECTORY
    ${FETCHCONTENT_BASE_DIR}/googletest-src/googletest/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ---- Export targets ----
install(EXPORT third_partyTargets
    NAMESPACE third_party::
    FILE third_partyTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/third_party
)

# ------------------------------------------------------------------------------
# Package config files
# ------------------------------------------------------------------------------

configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/third_partyConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/third_partyConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/third_party
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/third_partyConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/third_partyConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/third_partyConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/third_party
)