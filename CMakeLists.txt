cmake_minimum_required(VERSION 3.17.2)
project(raindrop)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# message("Current dir: ${CMAKE_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    include(cotire)
endif ()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    option(USE_GCC "Use GCC as your compiler" OFF)
    set(LINUX TRUE)
    add_definitions(/DLINUX)
    if (NOT USE_GCC)
        set(CMAKE_C_COMPILER   "clang")
        set(CMAKE_CXX_COMPILER "clang++")
        message("-- Compiling with clang...")
    else()
        set(CMAKE_C_COMPILER   "gcc")
        set(CMAKE_CXX_COMPILER "g++")
        message("-- Compiling with gcc...")    
    endif()
endif()

if(WIN32)
    add_compile_definitions(_UNICODE WIN32)
endif(WIN32)

# Configure those libraries. Find whenever possible.

# This is neccesary for CMake to try to find the library files from the bundle.
if (WIN32)
    # Includes
    set(BASE_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/lib/include")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/zlib")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/lua")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/png")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/jpeg")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/glfw")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/mpg123")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/libsndfile")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/soxr")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/portaudio")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/ffmpeg")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/RmlUi")

    # Libraries
    set(BASE_LIB_PATH "${CMAKE_SOURCE_DIR}/lib/lib")
    list(APPEND CMAKE_LIBRARY_PATH "${BASE_LIB_PATH}")
endif ()

# boost 1.75.0 on windows does not build the dll version of things.
if (WIN32)
    set(Boost_USE_STATIC_LIBS 1)
endif()

# While we use gil, there's no real need to include it here. it's header-only.
find_package(Boost COMPONENTS program_options date_time REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(GLEW REQUIRED)
find_package(Lua REQUIRED)
find_package(Portaudio REQUIRED)
find_package(JPEG REQUIRED)
find_package(OggVorbis REQUIRED)
find_package(MPG123 REQUIRED)
find_package(Sndfile REQUIRED)
find_package(Soxr REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(RmlUi REQUIRED)

# On linux, properly configured, these don't need our module scripts
# as they install their own Find modules.
if (LINUX)
    find_package(glfw3 CONFIG)
    find_package(libRocket CONFIG)
endif ()

# We may want to use our bundle, therefore...
if (WIN32)
    find_package(glfw3)
    # find_package(libRocket)
endif()

# message("GLFW3 directory: ${GLFW3_INCLUDE_DIR}")
# message("libRocket directory: ${libRocket_INCLUDE_DIRS}")

include_directories(include)
include_directories(src-thirdparty)
include_directories(${MPG123_INCLUDE_DIR})

set(SHA256_FILES src-thirdparty/sha256/sha256.cpp)
set(RINGBUFFER_FILES src-thirdparty/ringbuffer/pa_ringbuffer.c)
set(SQLITE_FILES src-thirdparty/sqlite/sqlite3.c)
set(MINIZIP_FILES src-thirdparty/minizip/ioapi.c src-thirdparty/minizip/unzip.c)

# Utility library
# ================================================
file(GLOB_RECURSE util_files src/util/*.cpp src/util/*.c)
add_library(rd.util STATIC ${util_files} ${SHA256_FILES})

if (LINUX)
    if (NOT USE_GCC)
        target_link_libraries(rd.util stdc++fs)
    else()
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.1)
            target_link_libraries(rd.util stdc++fs)
        else()
            find_package(Boost REQUIRED COMPONENTS filesystem)
            target_link_libraries(rd.util boost_filesystem)        
        endif()
    endif()
endif(LINUX)

if (USE_GCC)
cotire(rd.util)
endif()

# Sound File IO library
# ================================================
file(GLOB_RECURSE sndio_files "src/sndio/*.cpp")
add_library(rd.sndio STATIC ${sndio_files} ${RINGBUFFER_FILES})
target_include_directories(rd.sndio PUBLIC include/sndio)
target_include_directories(rd.sndio PUBLIC
        ${OGG_INCLUDE_DIR}
        ${VORBISFILE_INCLUDE_DIR}
        ${SNDFILE_INCLUDE_DIR}
        ${SOXR_INCLUDE_DIR})
target_compile_definitions(rd.sndio PUBLIC SNDIO_DLL)

#message(STATUS ${OGG_LIBRARY} " " ${VORBIS_LIBRARY} " "
#        ${VORBISFILE_LIBRARY})

target_link_libraries(rd.sndio
    rd.util
    ${OGG_LIBRARY} 
    ${VORBIS_LIBRARY} 
    ${VORBISFILE_LIBRARY}
    ${MPG123_LIBRARY}
    ${SNDFILE_LIBRARY}
    ${SOXR_LIBRARIES})


if (USE_GCC)
cotire(rd.util)
endif()

# Common chart data library
# ================================================
file(GLOB_RECURSE core_files src/game/*.cpp src/game/*.c)
add_library(rd.core STATIC ${core_files} ${MINIZIP_FILES})
target_include_directories(rd.core PUBLIC ${ZLIB_INCLUDE_DIR})
target_link_libraries(rd.core
    rd.sndio
    rd.util
    ${ZLIB_LIBRARIES})

if (USE_GCC)
cotire(rd.core)
endif()

# rd Body Source (What's common to Tests and the Actual Executable)
# ================================================
file(GLOB_RECURSE backend_files src/client/*.cpp)

set (BODY_SOURCE
        ${backend_files}
        ${SQLITE_FILES}
)

list(REMOVE_ITEM BODY_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/raindrop.cpp
)

add_library(rdbody.src
        ${BODY_SOURCE}
)

set(BODY_INCLUDE
        ${PNG_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${JPEG_INCLUDE_DIR}
        ${PORTAUDIO_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${LUA_INCLUDE_DIR}
        ${FFMPEG_INCLUDE_DIR}
        ${RMLUI_INCLUDE_DIR}
        src/client/backend
)

target_include_directories(rdbody.src PUBLIC
    ${BODY_INCLUDE}
)

if (WIN32)
    set(WINLIBS opengl32.lib Shlwapi.lib Dbghelp.lib)
    target_compile_definitions(rdbody.src PRIVATE -DUNICODE)
endif()

set (BODY_LIBS
        ${LUA_LIBRARIES}
        rd.core
        Boost::program_options
        Boost::date_time
        ${GLFW3_LIBRARIES}
        ${PNG_LIBRARIES}
        ${LUA_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${Portaudio_LIBRARIES}
        ${FFMPEG_LIBAVCODEC}
        ${FFMPEG_LIBAVUTIL}
        ${FFMPEG_LIBAVFORMAT}
        ${FFMPEG_LIBSWSCALE}
        ${RMLUI_LIBRARIES}
        ${WINLIBS})

target_link_libraries(rdbody.src
    ${BODY_LIBS}
)

# rdbody - the actual thing people can play
# ================================================
add_executable(rdbody
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/raindrop.cpp
)

target_link_libraries(rdbody rdbody.src)

if (WIN32)
    set(WINLIBS opengl32.lib Shlwapi.lib Dbghelp.lib)
    target_compile_definitions(rdbody PRIVATE -DUNICODE)
endif()

# Converter tools
# ================================================
add_executable(rdconverter
        src/converter/converter.cpp)

target_include_directories(rdconverter PRIVATE
        ${Boost_INCLUDE_DIR}
)

if (WIN32)
    set(WINLIBS_CONV Shlwapi.lib)
    target_compile_definitions(rdconverter PRIVATE -DUNICODE)
endif()

target_link_libraries(rdconverter
        rd.core
        Boost::program_options
        ${WINLIBS_CONV}
)

# Test runner
# ================================================


add_executable(rdtests
        tests/main.cpp
        tests/MechanicsTests.cpp
        tests/OsuManiaTests.cpp
        tests/NoteskinTests.cpp
        tests/FilesTests.cpp)

target_link_libraries(rdtests
    rdbody.src
)

target_include_directories(rdtests PRIVATE
        ${BODY_INCLUDE}
        src/
        src-thirdparty/
)