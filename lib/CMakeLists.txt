# ----------------------------------------------------------------------------- 
# lib/CMakeLists.txt
# Top-level CMake for internal libraries
# ----------------------------------------------------------------------------- 

# Use a namespace for all internal targets
# This makes it clear in downstream linking: rdemo_backup::<LibraryName>
add_subdirectory(BackupUtility)