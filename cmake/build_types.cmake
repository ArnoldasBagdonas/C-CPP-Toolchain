if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS
        Debug
        Release
        Coverage
        Valgrind
        AddressSanitizer
        ThreadSanitizer
        MemorySanitizer
        UndefinedBehaviorSanitizer
)

message(STATUS
    "Available build types: Debug, Release, Coverage, Valgrind, "
    "AddressSanitizer, ThreadSanitizer, MemorySanitizer, UndefinedBehaviorSanitizer"
)
