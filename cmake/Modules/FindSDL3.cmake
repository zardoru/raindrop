include(FindPackageHandleStandardArgs)
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

find_package(SDL3 CONFIG QUIET)

if(NOT TARGET SDL3::SDL3)
    set(SDL3_SOURCE_DIR "" CACHE PATH "Local SDL3 source directory")

    if(SDL3_SOURCE_DIR)
        FetchContent_Declare(SDL3 SOURCE_DIR "${SDL3_SOURCE_DIR}")
    else()
        FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG main
            GIT_SHALLOW TRUE
        )
    endif()

    FetchContent_MakeAvailable(SDL3)
endif()

if(TARGET SDL3::SDL3)
    set(SDL3_FOUND TRUE)
endif()

find_package_handle_standard_args(SDL3 DEFAULT_MSG SDL3_FOUND)
