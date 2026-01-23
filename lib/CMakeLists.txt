# ----------------------------------------------------------------------------- 
# lib/CMakeLists.txt
# Top-level CMake for internal libraries
# ----------------------------------------------------------------------------- 

# Use a namespace for all internal targets
# This makes it clear in downstream linking: rdemo_backup::<LibraryName>
add_subdirectory(TimestampProvider)
add_subdirectory(SnapshotDirectoryProvider)
add_subdirectory(FileHasher)
add_subdirectory(FileIterator)
add_subdirectory(ThreadedFileQueue)
add_subdirectory(SQLite)
add_subdirectory(BackupUtility)