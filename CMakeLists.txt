cmake_minimum_required(VERSION 3.22)

project(rdemo_backup
    VERSION 1.0
    LANGUAGES C CXX
)

# ----------------------------------------------------------------------------- 
# C++ Standard & Policies
# ----------------------------------------------------------------------------- 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure position-independent code for static/shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ----------------------------------------------------------------------------- 
# Build type setup
# ----------------------------------------------------------------------------- 
include(cmake/build_types.cmake)

# ----------------------------------------------------------------------------- 
# Compiler flags helper
# ----------------------------------------------------------------------------- 
include(cmake/compiler_flags.cmake)

# ----------------------------------------------------------------------------- 
# Third-party dependencies
# ----------------------------------------------------------------------------- 
# Prefer subdirectories over FetchContent when possible for reproducibility
add_subdirectory(third_party)

# ----------------------------------------------------------------------------- 
# Core library
# ----------------------------------------------------------------------------- 
add_subdirectory(lib)

# ----------------------------------------------------------------------------- 
# Main executable
# ----------------------------------------------------------------------------- 
add_executable(rdemo-backup src/main.cpp)
set_target_flags(rdemo-backup)

target_link_libraries(rdemo-backup
    PRIVATE
        rdemo_backup::BackupUtility   # Our internal library
        cxxopts::cxxopts              # Header-only options parser
        xxhash_static                 # Static hash library
        sqlite3
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

# ----------------------------------------------------------------------------- 
# Code coverage (only for Coverage build type)
# ----------------------------------------------------------------------------- 
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    if(MSVC)
        include(cmake/coverage/msvc_coverage.cmake)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        include(cmake/coverage/clang_coverage.cmake)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        include(cmake/coverage/gcc_coverage.cmake)
    endif()
endif()
