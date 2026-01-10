# =============================================================================
# third_party/CMakeLists.txt
# =============================================================================

include(FetchContent)

cmake_policy(SET CMP0135 NEW)

set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_LIST_DIR}/src)


set(FETCHCONTENT_QUIET OFF)


set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR})
set(DOWNLOAD_DIR ${THIRD_PARTY_DIR}/downloads)

file(MAKE_DIRECTORY ${DOWNLOAD_DIR})

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
        list(GET status 1 message)
        message(FATAL_ERROR "${name}: download failed: ${message}")
    endif()
endfunction()

# ------------------------------------------------------------------------------
# SQLite3 (amalgamation)
# ------------------------------------------------------------------------------
set(SQLITE_EXPECTED_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/sqlite3_src-src)

if(EXISTS ${SQLITE_EXPECTED_SOURCE_DIR})
    message(STATUS "sqlite3: Using existing source directory: ${SQLITE_EXPECTED_SOURCE_DIR}")
    FetchContent_Declare(
        sqlite3_src
        SOURCE_DIR ${SQLITE_EXPECTED_SOURCE_DIR}
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/sqlite3
        SUBBUILD_DIR ${CMAKE_BINARY_DIR}/third_party/sqlite3-subbuild
    )
else()
    message(STATUS "sqlite3: Source directory not found. Will download and extract.")
    set(SQLITE_VERSION 3510100)
    set(SQLITE_ARCHIVE sqlite-amalgamation-${SQLITE_VERSION}.zip)
    set(SQLITE_URL https://www.sqlite.org/2025/${SQLITE_ARCHIVE})
    set(SQLITE_ARCHIVE_PATH ${DOWNLOAD_DIR}/${SQLITE_ARCHIVE})

    download_if_missing(sqlite3 ${SQLITE_ARCHIVE_PATH} ${SQLITE_URL})

    FetchContent_Declare(
        sqlite3_src
        URL file://${SQLITE_ARCHIVE_PATH}
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/sqlite3
        SUBBUILD_DIR ${CMAKE_BINARY_DIR}/third_party/sqlite3-subbuild
    )
endif()
FetchContent_MakeAvailable(sqlite3_src)

add_library(sqlite3 STATIC
    ${sqlite3_src_SOURCE_DIR}/sqlite3.c
    ${sqlite3_src_SOURCE_DIR}/sqlite3.h
)

target_include_directories(sqlite3 PUBLIC
    ${sqlite3_src_SOURCE_DIR}
)

set_target_flags(sqlite3)

# ------------------------------------------------------------------------------
# cxxopts (header-only, v3.3.1)
# ------------------------------------------------------------------------------
set(CXXOPTS_EXPECTED_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/cxxopts-src)

if(EXISTS ${CXXOPTS_EXPECTED_SOURCE_DIR})
    message(STATUS "cxxopts: Using existing source directory: ${CXXOPTS_EXPECTED_SOURCE_DIR}")
    FetchContent_Declare(
        cxxopts
        SOURCE_DIR ${CXXOPTS_EXPECTED_SOURCE_DIR}
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/cxxopts
        SUBBUILD_DIR ${CMAKE_BINARY_DIR}/third_party/cxxopts-subbuild
    )
else()
    message(STATUS "cxxopts: Source directory not found. Will download and extract.")
    set(CXXOPTS_VERSION 3.3.1)
    set(CXXOPTS_ARCHIVE cxxopts-${CXXOPTS_VERSION}.tar.gz)
    set(CXXOPTS_URL https://github.com/jarro2783/cxxopts/archive/refs/tags/v${CXXOPTS_VERSION}.tar.gz)
    set(CXXOPTS_ARCHIVE_PATH ${DOWNLOAD_DIR}/${CXXOPTS_ARCHIVE})

    download_if_missing(cxxopts ${CXXOPTS_ARCHIVE_PATH} ${CXXOPTS_URL})

    FetchContent_Declare(
        cxxopts
        URL file://${CXXOPTS_ARCHIVE_PATH}
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/cxxopts
        SUBBUILD_DIR ${CMAKE_BINARY_DIR}/third_party/cxxopts-subbuild
    )
endif()
FetchContent_MakeAvailable(cxxopts)

# ------------------------------------------------------------------------------
# xxHash (v0.8.3)
# ------------------------------------------------------------------------------
set(XXHASH_EXPECTED_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/xxhash-src)

if(EXISTS ${XXHASH_EXPECTED_SOURCE_DIR})
    message(STATUS "xxhash: Using existing source directory: ${XXHASH_EXPECTED_SOURCE_DIR}")
    FetchContent_Declare(
        xxhash
        SOURCE_DIR ${XXHASH_EXPECTED_SOURCE_DIR}
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/xxhash
        SUBBUILD_DIR ${CMAKE_BINARY_DIR}/third_party/xxhash-subbuild
    )
else()
    message(STATUS "xxhash: Source directory not found. Will download and extract.")
    set(XXHASH_VERSION 0.8.3)
    set(XXHASH_ARCHIVE xxhash-${XXHASH_VERSION}.tar.gz)
    set(XXHASH_URL https://github.com/Cyan4973/xxHash/archive/refs/tags/v${XXHASH_VERSION}.tar.gz)
    set(XXHASH_ARCHIVE_PATH ${DOWNLOAD_DIR}/${XXHASH_ARCHIVE})

    download_if_missing(xxhash ${XXHASH_ARCHIVE_PATH} ${XXHASH_URL})

    FetchContent_Declare(
        xxhash
        URL file://${XXHASH_ARCHIVE_PATH}
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/xxhash
        SUBBUILD_DIR ${CMAKE_BINARY_DIR}/third_party/xxhash-subbuild
    )
endif()
FetchContent_MakeAvailable(xxhash)

add_library(xxhash_static STATIC
    ${xxhash_SOURCE_DIR}/xxhash.c
)

target_include_directories(xxhash_static PUBLIC
    ${xxhash_SOURCE_DIR}
)

set_target_flags(xxhash_static)

# =============================================================================
# GoogleTest (release-1.12.1)
# =============================================================================
set(GTEST_EXPECTED_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/googletest-src)

if(EXISTS ${GTEST_EXPECTED_SOURCE_DIR})
    message(STATUS "googletest: Using existing source directory: ${GTEST_EXPECTED_SOURCE_DIR}")
    FetchContent_Declare(
        googletest
        SOURCE_DIR ${GTEST_EXPECTED_SOURCE_DIR}
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/googletest
        SUBBUILD_DIR ${CMAKE_BINARY_DIR}/third_party/googletest-subbuild
    )
else()
    message(STATUS "googletest: Source directory not found. Will download and extract.")
    set(GTEST_VERSION 1.12.1)
    set(GTEST_ARCHIVE googletest-${GTEST_VERSION}.tar.gz)
    set(GTEST_URL https://github.com/google/googletest/archive/refs/tags/release-${GTEST_VERSION}.tar.gz)
    set(GTEST_ARCHIVE_PATH ${DOWNLOAD_DIR}/${GTEST_ARCHIVE})

    download_if_missing(googletest ${GTEST_ARCHIVE_PATH} ${GTEST_URL})

    FetchContent_Declare(
        googletest
        URL file://${GTEST_ARCHIVE_PATH}
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/googletest
        SUBBUILD_DIR ${CMAKE_BINARY_DIR}/third_party/googletest-subbuild
    )
endif()
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)