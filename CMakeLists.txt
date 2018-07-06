cmake_minimum_required(VERSION 3.5.1)
project(raindrop)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# message("Current dir: ${CMAKE_SOURCE_DIR}")

#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cotire)

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
    set(CMAKE_CXX_FLAGS    "-std=c++14")
endif()

if(WIN32)
    add_definitions(/DWIN32)
endif(WIN32)

# Configure those libraries. Find whenever possible.

find_package(Boost REQUIRED COMPONENTS system program_options)
find_package(ZLIB REQUIRED)
find_package(PNG)
find_package(GLEW)
find_package(Lua REQUIRED)
find_package(Portaudio)
find_package(JPEG)
find_package(glfw3)
find_package(OggVorbis REQUIRED)
find_package(MPG123 REQUIRED)
find_package(Sndfile REQUIRED)
find_package(libRocket)
find_package(Soxr REQUIRED)

# message("GLFW3 directory: ${GLFW3_INCLUDE_DIR}")
# message("libRocket directory: ${libRocket_INCLUDE_DIRS}")

include_directories(rd-include)
include_directories(lib/include)

# Utility library
# ================================================
file(GLOB_RECURSE util_files src-util/*.cpp src-util/*.c)
add_library(rd.util SHARED ${util_files})

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
file(GLOB_RECURSE sndio_files "src-sndio/*.cpp")
add_library(rd.sndio SHARED ${sndio_files})
target_include_directories(rd.sndio PUBLIC rd-include/sndio)
target_link_libraries(rd.sndio
    rd.util
    ${OGG_LIBRARY} 
    ${VORBIS_LIBRARY} 
    ${VORBISFILE_LIBRARY}
    ${MPG123_LIBRARY}
    ${SNDFILE_LIBRARY}
    ${SOXR_LIBRARY})

if (USE_GCC)
cotire(rd.util)
endif()
# Common chart data library
# ================================================
file(GLOB_RECURSE core_files src-gamecore/*.cpp src-gamecore/*.c)
add_library(rd.core SHARED ${core_files})
target_include_directories(rd.core PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(rd.core
    rd.sndio
    rd.util
    ${LUA_LIBRARIES} 
    ${ZLIB_LIBRARIES})

if (USE_GCC)
cotire(rd.core)
endif()

# Game Backend library
# ================================================
#file(GLOB_RECURSE backend_files src-backend/*.cpp)
#add_library(rd.backend SHARED ${backend_files})
#target_include_directories(rd.backend PRIVATE 
#    ${PNG_INCLUDE_DIRS}
#    ${GLEW_INCLUDE_DIRS}
#    ${JPEG_INCLUDE_DIR}
#    ${PORTAUDIO_INCLUDE_DIRS}
#)

# Scripter with raindrop API for doing cool things
file(GLOB_RECURSE scripter_files scripter/*.cpp)
add_executable(rd-script ${scripter_files})
target_link_libraries(rd-script rd.core rd.sndio)
target_include_directories(rd-script PRIVATE src-gamecore)

if (USE_GCC)
cotire(rd-script)
endif()
# target_link_libraries(rd.backend rd.core rd.sndio rd.util)


