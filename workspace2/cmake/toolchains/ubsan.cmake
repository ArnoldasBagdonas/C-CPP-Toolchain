# =============================================================================
# Toolchain: UndefinedBehaviorSanitizer
# =============================================================================
# Shared by both third_party and application builds to ensure identical
# instrumentation flags across the entire link chain.
# =============================================================================

set(CMAKE_C_FLAGS_INIT   "-fsanitize=undefined -fno-omit-frame-pointer -g")
set(CMAKE_CXX_FLAGS_INIT "-fsanitize=undefined -fno-omit-frame-pointer -g")

set(CMAKE_EXE_LINKER_FLAGS_INIT    "-fsanitize=undefined")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fsanitize=undefined")
