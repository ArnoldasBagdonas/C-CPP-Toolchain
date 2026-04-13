cmake_minimum_required(VERSION 3.22)

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

# Default location of prebuilt third_party
set(THIRD_PARTY_INSTALL_DIR
    "${CMAKE_SOURCE_DIR}/build/third_party/install"
    CACHE PATH "Path to third_party install directory"
)

# Add to search path
list(APPEND CMAKE_PREFIX_PATH "${THIRD_PARTY_INSTALL_DIR}")

# Try to find third_party package
find_package(third_party QUIET)

if(NOT third_party_FOUND)
    message(FATAL_ERROR
        "\nthird_party package not found!\n"
        "You must build it first:\n\n"
        "  cmake -S third_party -B build/third_party -G Ninja \\\n"
        "        -DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_DIR}\n"
        "  cmake --build build/third_party --target install\n"
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

# ----------------------------------------------------------------------------- 
# Clang-format support
# ----------------------------------------------------------------------------- 
include(cmake/clang_format.cmake)