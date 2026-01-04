# =============================================================================
# tests/CMakeLists.txt — Unit tests for rdemo_backup module
# =============================================================================

# ---------------------------------------------------------------------------
# Fetch Google Test
# ---------------------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_QUIET OFF) # verbose FetchContent for debugging

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# Test sources
# ---------------------------------------------------------------------------
set(TEST_SOURCES
    src/main.cpp
    src/backup_utility_test.cpp
)

# Optional sanitizer test sources
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
if(BUILD_TYPE_UPPER MATCHES "VALGRIND|ADDRESSSANITIZER|THREADSANITIZER|MEMORYSANITIZER|UNDEFINEDBEHAVIORSANITIZER")
     list(APPEND TEST_SOURCES src/sanitizer_test.cpp)
endif()

# ---------------------------------------------------------------------------
# Create test executable
# ---------------------------------------------------------------------------
add_executable(unit_tests ${TEST_SOURCES})

# Apply target-specific compiler/linker flags (defined in root CMake)
set_target_flags(unit_tests)

# Link dependencies
target_link_libraries(unit_tests
    PRIVATE
        gtest
        rdemo_backup::BackupUtility
)

# ---------------------------------------------------------------------------
# GoogleTest integration
# ---------------------------------------------------------------------------
include(GoogleTest)

# Configure sanitizer environment variables
set(SANITIZER_ENV "")
if(BUILD_TYPE_UPPER STREQUAL "ADDRESSSANITIZER")
    set(SANITIZER_ENV "ASAN_OPTIONS=abort_on_error=1:exitcode=42:detect_leaks=1:detect_invalid_pointer_pairs=1:detect_stack_use_after_return=1")
elseif(BUILD_TYPE_UPPER STREQUAL "THREADSANITIZER")
    set(SANITIZER_ENV "TSAN_OPTIONS=halt_on_error=1:exitcode=42")
elseif(BUILD_TYPE_UPPER STREQUAL "MEMORYSANITIZER")
    set(SANITIZER_ENV "MSAN_OPTIONS=abort_on_error=1:exitcode=42")
endif()

# Discover and run tests, one at a time for thread safety with sanitizers
gtest_discover_tests(unit_tests
    PROPERTIES
        ENVIRONMENT "${SANITIZER_ENV}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        RUN_SERIAL 1
        TIMEOUT 30
)

# ---------------------------------------------------------------------------
# Valgrind integration (Linux)
# ---------------------------------------------------------------------------
if(BUILD_TYPE_UPPER STREQUAL "VALGRIND")
    find_program(VALGRIND_EXECUTABLE valgrind REQUIRED)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(VALGRIND_XML "${CMAKE_CURRENT_BINARY_DIR}/valgrind.xml")

    # Wrapper to run tests under Valgrind
    add_custom_target(Valgrind.UnitTestsRun
        COMMAND ${CMAKE_COMMAND} -E echo "Running unit_tests under Valgrind (segfaults ignored)..."
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/run_valgrind.sh
                ${VALGRIND_EXECUTABLE}
                ${VALGRIND_XML}
                $<TARGET_FILE:unit_tests>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Run unit_tests under Valgrind ignoring segfaults"
        VERBATIM
    )
    set_property(TARGET Valgrind.UnitTestsRun PROPERTY FATAL_ERROR 0)

    # Parse Valgrind XML results
    add_custom_target(Valgrind.UnitTestsRun.ParseXML
        COMMAND ${CMAKE_COMMAND} -E echo "Parsing Valgrind XML..."
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/valgrindXML.py ${VALGRIND_XML}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Parsing Valgrind XML after tests"
        VERBATIM
    )
    add_dependencies(Valgrind.UnitTestsRun.ParseXML Valgrind.UnitTestsRun)
endif()
