# =============================================================================
# tests/CMakeLists.txt — Unit tests and end-to-end tests setup
# =============================================================================

include(GoogleTest)

# ---------------------------------------------------------------------------
# Configure sanitizer environment variables
# ---------------------------------------------------------------------------
set(SANITIZER_ENV "")
if(PROJECT_BUILD_VARIANT STREQUAL "ADDRESSSANITIZER")
    set(SANITIZER_ENV "ASAN_OPTIONS=abort_on_error=1:exitcode=42:detect_leaks=1:detect_invalid_pointer_pairs=1:detect_stack_use_after_return=1")
elseif(PROJECT_BUILD_VARIANT STREQUAL "THREADSANITIZER")
    set(SANITIZER_ENV "TSAN_OPTIONS=halt_on_error=1:exitcode=42")
elseif(PROJECT_BUILD_VARIANT STREQUAL "MEMORYSANITIZER")
    set(SANITIZER_ENV "MSAN_OPTIONS=abort_on_error=1:exitcode=42")
endif()

# ---------------------------------------------------------------------------
# Helper: add a GoogleTest executable with standard settings
# ---------------------------------------------------------------------------
function(setup_gtest_executable TARGET_NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})

    add_executable(${TARGET_NAME} ${ARG_SOURCES})

    target_link_libraries(${TARGET_NAME}
        PRIVATE
            third_party::gtest
            third_party::gmock
            ${ARG_LIBRARIES}
    )

    target_include_directories(${TARGET_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    set_target_flags(${TARGET_NAME})
endfunction()

# ---------------------------------------------------------------------------
# Helper: register GoogleTests in a uniform way
# ---------------------------------------------------------------------------
function(setup_test_discovery TARGET_NAME)
    gtest_discover_tests(${TARGET_NAME}
        PROPERTIES
            ENVIRONMENT "${SANITIZER_ENV}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            RUN_SERIAL 1
            TIMEOUT 30
    )
endfunction()


# ---------------------------------------------------------------------------
# Test sources
# ---------------------------------------------------------------------------
set(TEST_SOURCES
    src/main.cpp
    src/unit_tests.cpp
    src/e2e_tests.cpp
)

set(SANITIZER_TEST_SOURCES
    src/main.cpp
    src/sanitizer_validation_tests.cpp
)

# ---------------------------------------------------------------------------
# Define test executables
# ---------------------------------------------------------------------------
setup_gtest_executable(host_unit_tests
    SOURCES   ${TEST_SOURCES}
    LIBRARIES example::libexample
)

setup_gtest_executable(host_sanitizer_tests
    SOURCES ${SANITIZER_TEST_SOURCES}
)

# ---------------------------------------------------------------------------
# Register tests
# ---------------------------------------------------------------------------
setup_test_discovery(host_unit_tests)
if(NOT PROJECT_BUILD_VARIANT STREQUAL "COVERAGE")
    setup_test_discovery(host_sanitizer_tests)
endif()

# ---------------------------------------------------------------------------
# Valgrind integration (Linux)
# ---------------------------------------------------------------------------
if(PROJECT_BUILD_VARIANT STREQUAL "VALGRIND")
    find_program(VALGRIND_EXECUTABLE valgrind REQUIRED)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    # Function to create Valgrind test targets
    function(add_valgrind_test TARGET_NAME)
        set(VALGRIND_XML "${CMAKE_CURRENT_BINARY_DIR}/valgrind_${TARGET_NAME}.xml")

        # Wrapper to run tests under Valgrind
        add_custom_target(Valgrind.${TARGET_NAME}.Run
            COMMAND ${CMAKE_COMMAND} -E echo "Running ${TARGET_NAME} under Valgrind..."
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/run_valgrind.sh
                    ${VALGRIND_EXECUTABLE}
                    ${VALGRIND_XML}
                    $<TARGET_FILE:${TARGET_NAME}>
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Run ${TARGET_NAME} under Valgrind"
            VERBATIM
        )
        set_property(TARGET Valgrind.${TARGET_NAME}.Run PROPERTY FATAL_ERROR 0)

        # Parse Valgrind XML results
        add_custom_target(Valgrind.${TARGET_NAME}.ParseXML
            COMMAND ${CMAKE_COMMAND} -E echo "Parsing Valgrind XML for ${TARGET_NAME}..."
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/valgrindXML.py ${VALGRIND_XML}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Parsing Valgrind XML for ${TARGET_NAME}"
            VERBATIM
        )
        add_dependencies(Valgrind.${TARGET_NAME}.ParseXML Valgrind.${TARGET_NAME}.Run)
    endfunction()

    # Create Valgrind targets for both test executables
    add_valgrind_test(host_unit_tests)
    if(TARGET host_sanitizer_tests)
      add_valgrind_test(host_sanitizer_tests)
    endif()
endif()

# -----------------------------------------------------------------------------
# Code coverage
# -----------------------------------------------------------------------------

if(PROJECT_BUILD_VARIANT STREQUAL "COVERAGE")
    add_coverage_report(host_unit_tests)
endif()
