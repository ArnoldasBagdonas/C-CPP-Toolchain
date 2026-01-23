# -----------------------------------------------------------------------------
# lib/SQLite/CMakeLists.txt
# Build SQLite as a STATIC library with modern CMake practices
# -----------------------------------------------------------------------------

add_library(SQLite STATIC
    src/SQLiteSession.cpp
    src/SQLiteConnection.cpp
    src/SQLiteStatement.cpp
)

set_target_flags(SQLite)

target_include_directories(SQLite
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(SQLite
    PUBLIC
        sqlite3
)

add_library(rdemo_backup::SQLite ALIAS SQLite)
