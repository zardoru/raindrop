# - Try to find Portaudio
# Once done this will define
#
#  Portaudio_FOUND - system has Portaudio
#  Portaudio_INCLUDE_DIRS - the Portaudio include directory
#  Portaudio_LIBRARIES - Link these to use Portaudio

find_package(PkgConfig)
pkg_check_modules(Portaudio portaudio-2.0)

find_path(Portaudio_INCLUDE_DIRS
  NAMES
    portaudio.h
  PATHS
      /usr/local/include
      /usr/include
  HINTS
    ${PC_PORTAUDIO_INCLUDEDIR}
)

if (CMAKE_CL_64)
  set(PORTAUDIO_ARCH portaudio_x64)
else()
  set(PORTAUDIO_ARCH portaudio_x32)
endif()


find_library(Portaudio_LIBRARIES
  NAMES
    portaudio
    ${PORTAUDIO_ARCH}
  PATHS
      /usr/local/lib
      /usr/lib
      /usr/lib64
  HINTS
    ${PC_PORTAUDIO_LIBDIR}
)

mark_as_advanced(Portaudio_INCLUDE_DIRS Portaudio_LIBRARIES)

# Found PORTAUDIO, but it may be version 18 which is not acceptable.
if(EXISTS ${Portaudio_INCLUDE_DIRS}/portaudio.h)
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_INCLUDES_SAVED ${CMAKE_REQUIRED_INCLUDES})
  set(CMAKE_REQUIRED_INCLUDES ${Portaudio_INCLUDE_DIRS})
  CHECK_CXX_SOURCE_COMPILES(
    "#include <portaudio.h>\nPaDeviceIndex pa_find_device_by_name(const char *name); int main () {return 0;}"
    PORTAUDIO2_FOUND)
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_SAVED})
  unset(CMAKE_REQUIRED_INCLUDES_SAVED)
  if(PORTAUDIO2_FOUND)
    INCLUDE(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(Portaudio DEFAULT_MSG Portaudio_INCLUDE_DIRS Portaudio_LIBRARIES)
    set(Portaudio_FOUND TRUE)
  else(PORTAUDIO2_FOUND)
    message(STATUS
      "  portaudio.h not compatible (requires API 2.0)")
    set(Portaudio_FOUND FALSE)
  endif(PORTAUDIO2_FOUND)
endif()
