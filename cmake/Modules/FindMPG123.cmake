# This will define
# MPG123_FOUND
# MPG123_INCLUDE_DIR
# MPG123_LIBRARY

include(FetchContent)
include(FindPackageHandleStandardArgs)

find_path(MPG123_INCLUDE_DIR 
            NAMES mpg123.h
            HINTS /usr/include
)

find_library(MPG123_LIBRARY NAMES mpg123 libmpg123)

if(MPG123_INCLUDE_DIR AND MPG123_LIBRARY AND NOT TARGET MPG123::MPG123)
    add_library(MPG123::MPG123 UNKNOWN IMPORTED)
    set_target_properties(MPG123::MPG123 PROPERTIES
        IMPORTED_LOCATION "${MPG123_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MPG123_INCLUDE_DIR}"
    )
endif()

if(NOT TARGET MPG123::MPG123)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    set(BUILD_PROGRAMS OFF CACHE BOOL "Build mpg123 programs" FORCE)

    FetchContent_Declare(mpg123
        GIT_REPOSITORY https://github.com/libsdl-org/mpg123.git
        GIT_TAG main
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR ports/cmake
    )

    FetchContent_MakeAvailable(mpg123)
    set(MPG123_FETCH_INCLUDE_DIR "${mpg123_SOURCE_DIR}/src/include")

    foreach(MPG123_TARGET_CANDIDATE
            MPG123::libmpg123
            mpg123::libmpg123
            libmpg123
            mpg123)
        if(TARGET "${MPG123_TARGET_CANDIDATE}" AND NOT TARGET MPG123::MPG123)
            get_target_property(MPG123_ALIASED_TARGET "${MPG123_TARGET_CANDIDATE}" ALIASED_TARGET)
            if(MPG123_ALIASED_TARGET)
                set(MPG123_TARGET "${MPG123_ALIASED_TARGET}")
            else()
                set(MPG123_TARGET "${MPG123_TARGET_CANDIDATE}")
            endif()

            target_include_directories("${MPG123_TARGET}" PUBLIC "$<BUILD_INTERFACE:${MPG123_FETCH_INCLUDE_DIR}>")
            add_library(MPG123::MPG123 ALIAS "${MPG123_TARGET}")
            set(MPG123_LIBRARY MPG123::MPG123)
            set(MPG123_INCLUDE_DIR "${MPG123_FETCH_INCLUDE_DIR}")
        endif()
    endforeach()
endif()

if(TARGET MPG123::MPG123)
    set(MPG123_FOUND TRUE)
    set(MPG123_LIBRARY MPG123::MPG123)
endif()

find_package_handle_standard_args(MPG123 DEFAULT_MSG MPG123_FOUND)

mark_as_advanced(MPG123_INCLUDE_DIR MPG123_LIBRARY)

set(MPG123_LIBRARIES ${MPG123_LIBRARY})
set(MPG123_INCLUDE_DIRS ${MPG123_INCLUDE_DIR})
