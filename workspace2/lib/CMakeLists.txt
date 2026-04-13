# ----------------------------------------------------------------------------- 
# lib/CMakeLists.txt
# Top-level CMake for internal libraries
# ----------------------------------------------------------------------------- 

# Reference the shared library source directly from the root project.
# add_subdirectory(<source_dir> <binary_dir>) lets CMake build an
# out-of-tree source directory while keeping outputs inside host-app's
# build tree. This avoids copying source files for coverage.
set(EXTERNAL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../lib")

add_subdirectory("${EXTERNAL_LIB_DIR}/example"
                 "${CMAKE_CURRENT_BINARY_DIR}/example")

# Register the external source tree so coverage tools (e.g. OpenCppCoverage)
# can instrument files that live outside CMAKE_SOURCE_DIR.
set_property(GLOBAL APPEND PROPERTY COVERAGE_ADDITIONAL_SOURCE_DIRS
    "${EXTERNAL_LIB_DIR}"
)