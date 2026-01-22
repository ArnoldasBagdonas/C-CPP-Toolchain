# ----------------------------------------------------------------------------- 
# lib/BackupUtility/CMakeLists.txt
# Build BackupUtility as a STATIC library with modern CMake practices
# ----------------------------------------------------------------------------- 

# Create the static library
add_library(BackupUtility STATIC
    src/BackupUtility.cpp
    src/TimestampProvider.cpp
    src/FilesystemFileEnumerator.cpp
    src/FilesystemSnapshotDirectory.cpp
    src/FileStateRepository.cpp
    src/ProcessBackupFile.cpp
    src/ProcessDeletedFiles.cpp
    src/SQLiteConnection.cpp
    src/SQLiteSession.cpp
    src/SQLiteStatement.cpp
    src/SnapshotDirectoryProvider.cpp
    src/ThreadedFileQueue.cpp
    src/FileHasher.cpp
)

# Apply compiler flags for build type (Debug/Release/Coverage/Valgrind)
set_target_flags(BackupUtility)

# Include directories
target_include_directories(BackupUtility
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies (internal only)
target_link_libraries(BackupUtility
    PRIVATE
        sqlite3
        xxhash_static
)

# Namespace alias for public-facing usage
add_library(rdemo_backup::BackupUtility ALIAS BackupUtility)