# -----------------------------------------------------------------------------
# lib/SnapshotDirectoryProvider/CMakeLists.txt
# Build SnapshotDirectoryProvider as a STATIC library with modern CMake practices
# -----------------------------------------------------------------------------

add_library(SnapshotDirectoryProvider STATIC
    src/SnapshotDirectoryProvider.cpp
)

set_target_flags(SnapshotDirectoryProvider)

target_include_directories(SnapshotDirectoryProvider
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(SnapshotDirectoryProvider
    PUBLIC
    TimestampProvider
)

add_library(rdemo_backup::SnapshotDirectoryProvider ALIAS SnapshotDirectoryProvider)
