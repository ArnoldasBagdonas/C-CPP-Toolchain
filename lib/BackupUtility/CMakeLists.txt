# ----------------------------------------------------------------------------- 
# lib/BackupUtility/CMakeLists.txt
# Build BackupUtility as a STATIC library with modern CMake practices
# ----------------------------------------------------------------------------- 

# Create the static library
add_library(BackupUtility STATIC
    src/BackupUtility.cpp
    src/FileStateRepository.cpp
    src/ProcessBackupFile.cpp
    src/ProcessDeletedFiles.cpp
)

# Apply compiler flags for build type (Debug/Release/Coverage/Valgrind)
set_target_flags(BackupUtility)

# Include directories
target_include_directories(BackupUtility
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies (internal only)
target_link_libraries(BackupUtility
    PUBLIC
        FileHasher
        FileIterator
        SnapshotDirectoryProvider
        SQLite
        ThreadedFileQueue
        TimestampProvider
)

# Namespace alias for public-facing usage
add_library(rdemo_backup::BackupUtility ALIAS BackupUtility)