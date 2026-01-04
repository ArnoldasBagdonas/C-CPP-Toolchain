function(set_target_flags target)

    if(NOT CMAKE_BUILD_TYPE)
        message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set")
    endif()

    string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_ID)

    # -------------------------------------------------------------------------
    # Common debug info (safe everywhere)
    # -------------------------------------------------------------------------
    if(NOT MSVC)
        target_compile_options(${target} PRIVATE -g)
    endif()

    # -------------------------------------------------------------------------
    # DEBUG
    # -------------------------------------------------------------------------
    if(BUILD_ID STREQUAL "DEBUG")
        if(MSVC)
            target_compile_options(${target} PRIVATE /Zi /Od)
            target_link_options(${target} PRIVATE /DEBUG)
        else()
            target_compile_options(${target} PRIVATE -O0)
        endif()

    # -------------------------------------------------------------------------
    # RELEASE
    # -------------------------------------------------------------------------
    elseif(BUILD_ID STREQUAL "RELEASE")
        if(MSVC)
            target_compile_options(${target} PRIVATE /O2 /DNDEBUG)
        else()
            target_compile_options(${target} PRIVATE -O3 -DNDEBUG)
        endif()

    # -------------------------------------------------------------------------
    # COVERAGE
    # -------------------------------------------------------------------------
    elseif(BUILD_ID STREQUAL "COVERAGE")
        if(MSVC)
            # MSVC coverage is handled externally via OpenCppCoverage
            # No special compiler or linker flags required
            target_compile_options(${target} PRIVATE /Zi /Od)
            target_link_options(${target} PRIVATE /DEBUG)
        elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} PRIVATE
                -O0
                -fprofile-instr-generate
                -fcoverage-mapping
                -fno-omit-frame-pointer
            )
            target_link_options(${target} PRIVATE
                -fprofile-instr-generate
                -fcoverage-mapping
            )
        elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE -O0 --coverage)
            target_link_options(${target} PRIVATE --coverage)
        endif()

    # -------------------------------------------------------------------------
    # VALGRIND
    # -------------------------------------------------------------------------
    elseif(BUILD_ID STREQUAL "VALGRIND")
        if(MSVC)
            message(FATAL_ERROR "Valgrind requires clang-cl or GCC")
        else()
            target_compile_options(${target} PRIVATE
                -O0
                -fno-omit-frame-pointer
            )

            # --- Clang-specific fix for Valgrind ---
            if(CMAKE_C_COMPILER_ID MATCHES "Clang")
                message(STATUS "Clang detected for Valgrind: forcing DWARF2 for Valgrind compatibility")
                target_compile_options(${target} PRIVATE -gdwarf-2)
            endif()
        endif()

    # -------------------------------------------------------------------------
    # ADDRESS SANITIZER
    # -------------------------------------------------------------------------
    elseif(BUILD_ID STREQUAL "ADDRESSSANITIZER")
        if(MSVC)
            message(FATAL_ERROR "AddressSanitizer requires clang-cl or GCC")
        endif()

        target_compile_options(${target} PRIVATE
            -O0
            -fsanitize=address
            -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE
            -fsanitize=address
        )

    # -------------------------------------------------------------------------
    # THREAD SANITIZER
    # -------------------------------------------------------------------------
    elseif(BUILD_ID STREQUAL "THREADSANITIZER")
        if(MSVC)
            message(FATAL_ERROR "ThreadSanitizer is not supported by MSVC")
        endif()

        find_package(Threads REQUIRED)

        target_compile_options(${target} PRIVATE
            -O1
            -fsanitize=thread
            -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE
            -fsanitize=thread
        )
        target_link_libraries(${target} PRIVATE Threads::Threads)

    # -------------------------------------------------------------------------
    # MEMORY SANITIZER
    # -------------------------------------------------------------------------
    elseif(BUILD_ID STREQUAL "MEMORYSANITIZER")
        if(MSVC OR NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
            message(FATAL_ERROR "MemorySanitizer requires Clang")
        endif()

        target_compile_options(${target} PRIVATE
            -O1
            -fsanitize=memory
            -fno-omit-frame-pointer
        )
        target_link_options(${target} PRIVATE
            -fsanitize=memory
        )

    # -------------------------------------------------------------------------
    # UNDEFINED BEHAVIOR SANITIZER
    # -------------------------------------------------------------------------
    elseif(BUILD_ID STREQUAL "UNDEFINEDBEHAVIORSANITIZER")
        set(UBSAN_FLAGS
            -fsanitize=undefined
            -fsanitize=shift
            -fsanitize=integer-divide-by-zero
            -fsanitize=unreachable
            -fsanitize=vla-bound
            -fsanitize=null
            -fsanitize=return
            -fsanitize=signed-integer-overflow
            -fsanitize=bounds
            -fsanitize=alignment
            -fsanitize=float-divide-by-zero
            -fsanitize=float-cast-overflow
            -fsanitize=nonnull-attribute
            -fsanitize=returns-nonnull-attribute
            -fsanitize=bool
            -fsanitize=enum
            -fsanitize=vptr
        )

        target_compile_options(${target} PRIVATE
            -O0
            ${UBSAN_FLAGS}
            -fno-sanitize-recover=all
            -fno-omit-frame-pointer
        )

        target_link_options(${target} PRIVATE
            ${UBSAN_FLAGS}
            -fno-sanitize-recover=all
        )

    else()
        message(FATAL_ERROR "Unknown build type: ${CMAKE_BUILD_TYPE}")
    endif()

endfunction()
