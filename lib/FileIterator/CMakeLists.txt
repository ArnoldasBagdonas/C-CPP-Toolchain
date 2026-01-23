# -----------------------------------------------------------------------------
# lib/FileIterator/CMakeLists.txt
# Build FileIterator as a STATIC library with modern CMake practices
# -----------------------------------------------------------------------------

add_library(FileIterator STATIC
    src/FileIterator.cpp
)

set_target_flags(FileIterator)

target_include_directories(FileIterator
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_library(rdemo_backup::FileIterator ALIAS FileIterator)
