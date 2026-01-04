# Use SQLite3 amalgamation (recommended on Windows)

Download the single-file sqlite3.c + sqlite3.h from:
https://www.sqlite.org/download.html

(e.g. https://www.sqlite.org/2025/sqlite-amalgamation-3510100.zip)

Put them in third_party/sqlite3/ and in CMake:

```
add_library(sqlite3 STATIC
    third_party/sqlite3/sqlite3.c
    third_party/sqlite3/sqlite3.h
)

target_include_directories(sqlite3 PUBLIC
    third_party/sqlite3
)
```