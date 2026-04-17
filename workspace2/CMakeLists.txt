cmake_minimum_required(VERSION 3.25)

project(example-app
    VERSION 1.0
    LANGUAGES C CXX
)

# ----------------------------------------------------------------------------- 
# C++ Standard & Policies
# ----------------------------------------------------------------------------- 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------------------------------------------- 
# Build type setup
# ----------------------------------------------------------------------------- 
include(cmake/build_types.cmake)

if(CMAKE_BUILD_TYPE)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" PROJECT_BUILD_VARIANT)
else()
    set(PROJECT_BUILD_VARIANT "")
endif()

# ----------------------------------------------------------------------------- 
# Compiler flags helper
# ----------------------------------------------------------------------------- 
include(cmake/compiler_flags.cmake)

# ----------------------------------------------------------------------------- 
# Code coverage helper
# ----------------------------------------------------------------------------- 
include(cmake/coverage/coverage_report.cmake)

# ----------------------------------------------------------------------------- 
# Third-party dependencies (PREBUILT)
# -----------------------------------------------------------------------------
# Auto-detect the correct third_party install directory based on the active
# compiler and build type. The Makefile / CMake presets install third_party
# into per-compiler, per-variant directories:
#
#   build/third_party/gcc-default/install
#   build/third_party/clang-asan/install
#   build/third_party/msvc-default/install
#   ...
# -----------------------------------------------------------------------------

# Determine compiler prefix
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(_TP_COMPILER "gcc")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(_TP_COMPILER "clang")
elseif(MSVC)
    set(_TP_COMPILER "msvc")
else()
    set(_TP_COMPILER "unknown")
endif()

# Determine variant suffix from build type
string(TOUPPER "${CMAKE_BUILD_TYPE}" _TP_BUILD_UPPER)
if(_TP_BUILD_UPPER STREQUAL "ADDRESSSANITIZER")
    set(_TP_VARIANT "asan")
elseif(_TP_BUILD_UPPER STREQUAL "THREADSANITIZER")
    set(_TP_VARIANT "tsan")
elseif(_TP_BUILD_UPPER STREQUAL "MEMORYSANITIZER")
    set(_TP_VARIANT "msan")
elseif(_TP_BUILD_UPPER STREQUAL "UNDEFINEDBEHAVIORSANITIZER")
    set(_TP_VARIANT "ubsan")
else()
    set(_TP_VARIANT "default")
endif()

# BUILD_ROOT must be set — either via environment variable or cache variable.
# The single source of truth is remoteEnv.BUILD_ROOT in .devcontainer/devcontainer.json.
if(NOT BUILD_ROOT AND DEFINED ENV{BUILD_ROOT})
    set(BUILD_ROOT "$ENV{BUILD_ROOT}")
endif()
if(NOT BUILD_ROOT)
    message(FATAL_ERROR
        "\nBUILD_ROOT is not set.\n\n"
        "Set it via one of:\n"
        "  1. Environment variable (set by devcontainer): export BUILD_ROOT=/tmp/build\n"
        "  2. Cache variable: cmake -DBUILD_ROOT=/tmp/build ...\n\n"
        "The canonical value is remoteEnv.BUILD_ROOT in .devcontainer/devcontainer.json\n"
    )
endif()

set(THIRD_PARTY_INSTALL_DIR
    "${BUILD_ROOT}/third_party/${_TP_COMPILER}-${_TP_VARIANT}/install"
    CACHE PATH "Path to third_party install directory"
)
message(STATUS "Third-party install dir: ${THIRD_PARTY_INSTALL_DIR}")

# Add to search path
list(APPEND CMAKE_PREFIX_PATH "${THIRD_PARTY_INSTALL_DIR}")

# Try to find third_party package
find_package(third_party QUIET)

if(NOT third_party_FOUND)
    message(FATAL_ERROR
        "\nthird_party package not found at:\n"
        "  ${THIRD_PARTY_INSTALL_DIR}\n\n"
        "Build it first with:\n\n"
        "  make third-party\n\n"
        "Or manually with CMake presets:\n\n"
        "  cmake --preset ${_TP_COMPILER}-${_TP_VARIANT} -S third_party\n"
        "  cmake --build --preset ${_TP_COMPILER}-${_TP_VARIANT} -S third_party\n"
    )
endif()

# ----------------------------------------------------------------------------- 
# Core library
# ----------------------------------------------------------------------------- 
add_subdirectory(lib)

# ----------------------------------------------------------------------------- 
# Main executable
# ----------------------------------------------------------------------------- 
add_executable(example-app src/main.cpp)
set_target_flags(example-app)

target_link_libraries(example-app
    PRIVATE
        example::libexample
)

# ----------------------------------------------------------------------------- 
# Enable testing
# ----------------------------------------------------------------------------- 
include(CTest)
enable_testing()
add_subdirectory(tests)
