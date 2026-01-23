# -----------------------------------------------------------------------------
# lib/SQLiteSession/CMakeLists.txt
# Build SQLiteSession as a STATIC library with modern CMake practices
# -----------------------------------------------------------------------------

add_library(SQLiteSession STATIC
    src/SQLiteSession.cpp
    src/SQLiteConnection.cpp
    src/SQLiteStatement.cpp
)

set_target_flags(SQLiteSession)

target_include_directories(SQLiteSession
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(SQLiteSession
    PUBLIC
        sqlite3
)

add_library(rdemo_backup::SQLiteSession ALIAS SQLiteSession)
