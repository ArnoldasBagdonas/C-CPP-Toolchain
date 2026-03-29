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

# Normalize build type once, project-wide
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
# Third-party dependencies
# ----------------------------------------------------------------------------- 
# Prefer subdirectories over FetchContent when possible for reproducibility and build speed
add_subdirectory(third_party)

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
        example::libexample   # example library
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



