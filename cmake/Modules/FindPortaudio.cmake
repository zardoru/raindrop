include(FetchContent)
include(FindPackageHandleStandardArgs)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(portaudio-2.0 IMPORTED_TARGET portaudio-2.0)
endif()

if (NOT TARGET PkgConfig::portaudio-2.0)
    find_path(Portaudio_INCLUDE_DIRS
            NAMES
            portaudio.h
            PATHS
            /usr/local/include
            /usr/include
            HINTS
            ${PC_PORTAUDIO_INCLUDEDIR}
    )

    find_library(Portaudio_LIBRARIES
            NAMES
            portaudio
            PATHS
            /usr/local/lib
            /usr/lib
            /usr/lib64
            HINTS
            ${PC_PORTAUDIO_LIBDIR}
    )
endif ()

mark_as_advanced(Portaudio_INCLUDE_DIRS Portaudio_LIBRARIES)

# Found PORTAUDIO, but it may be version 18 which is not acceptable.
if (EXISTS ${Portaudio_INCLUDE_DIRS}/portaudio.h)
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES_SAVED ${CMAKE_REQUIRED_INCLUDES})
    set(CMAKE_REQUIRED_INCLUDES ${Portaudio_INCLUDE_DIRS})
    CHECK_CXX_SOURCE_COMPILES(
            "#include <portaudio.h>\nPaDeviceIndex pa_find_device_by_name(const char *name); int main () {return 0;}"
            PORTAUDIO2_FOUND)
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_SAVED})
    unset(CMAKE_REQUIRED_INCLUDES_SAVED)
    if (PORTAUDIO2_FOUND)
        set(Portaudio_FOUND TRUE)
    else (PORTAUDIO2_FOUND)
        message(STATUS
                "  portaudio.h not compatible (requires API 2.0)")
        set(Portaudio_FOUND FALSE)
    endif (PORTAUDIO2_FOUND)
endif ()

if (Portaudio_FOUND AND NOT TARGET PortAudio::PortAudio)
    # make a funny alias here
    add_library(PortAudio::PortAudio ALIAS PkgConfig::portaudio-2.0)
endif ()

if (NOT TARGET PortAudio::PortAudio)
    FetchContent_Declare(
            portaudio
            GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
            GIT_TAG master
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(portaudio)

    if (TARGET PortAudio::PortAudio)
        set(Portaudio_FOUND TRUE)
    elseif (TARGET portaudio)
        add_library(PortAudio::PortAudio ALIAS portaudio)
        set(Portaudio_FOUND TRUE)
    elseif (TARGET portaudio_static)
        add_library(PortAudio::PortAudio ALIAS portaudio_static)
        set(Portaudio_FOUND TRUE)
    endif ()
endif ()
