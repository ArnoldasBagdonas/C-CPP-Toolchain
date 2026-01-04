# third_party/CMakeLists.txt
# Clone third party repositories into third_party folder (manually or via Git submodule)

# ----------------------------------------------------------------------------- 
# SQLite3
# ----------------------------------------------------------------------------- 

# Create SQLite3 as an STATIC library
add_library(sqlite3 STATIC
    sqlite3/sqlite3.c
)

# Apply compiler flags (Debug/Release/Coverage/Valgrind)
set_target_flags(sqlite3)

# Include directories
target_include_directories(sqlite3 PUBLIC
    sqlite3
)

