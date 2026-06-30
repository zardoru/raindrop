include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

find_package(SndFile CONFIG QUIET)

if(TARGET SndFile::sndfile)
    set(SNDFILE_FOUND TRUE)
else()
    find_path(SNDFILE_INCLUDE_DIR sndfile.h
                HINTS
                 /usr/include)

    find_library(SNDFILE_LIBRARY NAMES libsndfile sndfile libsndfile-1 sndfile-1)

    if(SNDFILE_LIBRARY AND SNDFILE_INCLUDE_DIR AND NOT TARGET SndFile::sndfile)
        add_library(SndFile::sndfile UNKNOWN IMPORTED)
        set_target_properties(SndFile::sndfile PROPERTIES
            IMPORTED_LOCATION "${SNDFILE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SNDFILE_INCLUDE_DIR}"
        )
    endif()
endif()

if(NOT TARGET SndFile::sndfile)
    FetchContent_Declare(
        libsndfile
        GIT_REPOSITORY https://github.com/libsndfile/libsndfile.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(libsndfile)

    if(TARGET sndfile AND NOT TARGET SndFile::sndfile)
        add_library(SndFile::sndfile ALIAS sndfile)
    endif()
endif()

if(TARGET SndFile::sndfile)
    set(SNDFILE_FOUND TRUE)
    set(SNDFILE_LIBRARIES SndFile::sndfile)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sndfile DEFAULT_MSG SNDFILE_FOUND)

mark_as_advanced(SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY)
set(SNDFILE_INCLUDE_DIRS ${SNDFILE_INCLUDE_DIR})
