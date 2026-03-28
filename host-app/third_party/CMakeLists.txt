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