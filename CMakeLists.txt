cmake_minimum_required(VERSION 4.2)
project(
        raindrop
        VERSION 0.700.0
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option(BUILD_TESTS "build tests?" ON)

# message("Current dir: ${CMAKE_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

execute_process(
        COMMAND git rev-parse --short HEAD
        OUTPUT_VARIABLE GIT_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(
        config.h.in
        src/config.h
)

## <== CPR SNIP
include(FetchContent)
# Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG f091b2c061b307ee89b164c39976fc9202a1c79d)
FetchContent_MakeAvailable(cpr)
## CPR SNIP ==>

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
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/mpg123")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/libsndfile")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/portaudio")
    list(APPEND CMAKE_INCLUDE_PATH "${BASE_INCLUDE_PATH}/ffmpeg")

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
find_package(SDL3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(GLEW REQUIRED)
find_package(Lua REQUIRED)
find_package(JPEG REQUIRED)
find_package(OggVorbis REQUIRED)
find_package(MPG123 REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(Portaudio REQUIRED)
find_package(Sndfile REQUIRED)

if (UNIX)
    find_package(Iconv REQUIRED)
endif()

if (BUILD_TESTS)
    include(CTest)
    enable_testing()

    find_package(Catch2 3)

    if (NOT TARGET Catch2::Catch2)
        FetchContent_Declare(
                Catch2
                GIT_REPOSITORY https://github.com/catchorg/Catch2.git
                GIT_TAG        v3.8.1 # or a later release
        )

        FetchContent_MakeAvailable(Catch2)
        list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    endif()
endif()

message("$<==== BOOST ${Boost_PROGRAM_OPTIONS_LIBRARY} =====>")



add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/libotoworm" "${CMAKE_BINARY_DIR}/_deps/libotoworm")

include_directories(include)
include_directories(src-thirdparty)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
include_directories(${MPG123_INCLUDE_DIR})

set(RINGBUFFER_FILES src-thirdparty/ringbuffer/pa_ringbuffer.c)
set(SQLITE_FILES src-thirdparty/sqlite/sqlite3.c)
set(MINIZIP_FILES src-thirdparty/minizip/ioapi.c src-thirdparty/minizip/unzip.c)

# Sound File IO library
# ================================================
file(GLOB_RECURSE sndio_files CONFIGURE_DEPENDS "src/sndio/*.cpp")
add_library(rd.sndio STATIC ${sndio_files} ${RINGBUFFER_FILES})
target_include_directories(rd.sndio PUBLIC include/sndio)
target_include_directories(rd.sndio PUBLIC
        ${OGG_INCLUDE_DIR}
        ${VORBISFILE_INCLUDE_DIR}
        ${SNDFILE_INCLUDE_DIR}
        ${FFMPEG_INCLUDE_DIR})
target_compile_definitions(rd.sndio PUBLIC SNDIO_DLL)

#message(STATUS ${OGG_LIBRARY} " " ${VORBIS_LIBRARY} " "
#        ${VORBISFILE_LIBRARY})

target_link_libraries(rd.sndio
    otoworm::otoworm
    ${OGG_LIBRARY} 
    ${VORBIS_LIBRARY} 
    ${VORBISFILE_LIBRARY}
    ${MPG123_LIBRARY}
    SndFile::sndfile
    SDL3::SDL3)


# Common chart data library
# ================================================
file(GLOB_RECURSE core_files CONFIGURE_DEPENDS src/game/*.cpp src/game/*.c)
add_library(rd.core STATIC ${core_files} ${MINIZIP_FILES})
target_include_directories(rd.core PUBLIC ${ZLIB_INCLUDE_DIR})
target_link_libraries(rd.core
    otoworm::otoworm
    rd.sndio
    ZLIB::ZLIB)

# rd Body Source (What's common to Tests and the Actual Executable)
# ================================================
file(GLOB_RECURSE backend_files CONFIGURE_DEPENDS src/client/*.cpp)

set (BODY_SOURCE
        ${backend_files}
        ${SQLITE_FILES}
        src/ir/StormIR.cpp src/ir/StormIR.h src/ir/ScoreSerializer.cpp src/ir/ScoreSerializer.h)

list(REMOVE_ITEM BODY_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client/raindrop.cpp
)

add_library(rdbody.src STATIC
        ${BODY_SOURCE}
)

set(BODY_INCLUDE
        ${PNG_INCLUDE_DIRS}
        ${JPEG_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
        ${LUA_INCLUDE_DIR}
        ${FFMPEG_INCLUDE_DIR}
        src/client/backend
)

target_include_directories(rdbody.src PUBLIC
    ${BODY_INCLUDE}
)

if (WIN32)
    set(WINLIBS opengl32.lib Shlwapi.lib Dbghelp.lib)
    target_compile_definitions(rdbody.src PRIVATE -DUNICODE)
endif()

target_link_libraries(rdbody.src
        ${LUA_LIBRARIES}
        rd.core
        Boost::program_options
        Boost::date_time
        SDL3::SDL3
        ${PNG_LIBRARIES}
        ${LUA_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${GLEW_LIBRARIES}
        PortAudio::PortAudio
        ${FFMPEG_LIBAVCODEC}
        ${FFMPEG_LIBAVUTIL}
        ${FFMPEG_LIBAVFORMAT}
        ${FFMPEG_LIBSWSCALE}
        ${WINLIBS}
        cpr::cpr)

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
        otoworm::otoworm
        Boost::program_options
        ${WINLIBS_CONV}
)

# Test runner
# ================================================
if (TARGET Catch2::Catch2)

    add_executable(rdtests
            tests/main.cpp
            tests/MechanicsTests.cpp
            tests/OsuManiaTests.cpp
            tests/NoteskinTests.cpp
            tests/FilesTests.cpp)

    target_link_libraries(rdtests
        PRIVATE
            rdbody.src
            Catch2::Catch2
    )

    target_include_directories(rdtests PRIVATE
            ${BODY_INCLUDE}
            src/
            src-thirdparty/
    )

    include(Catch)
    catch_discover_tests(rdtests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
else()
    message("can't build tests, no catch2.")
endif()

install(TARGETS rdconverter rdbody)
install(DIRECTORY data/ DESTINATION data)
install(FILES default_config.ini RENAME config.ini DESTINATION .)
