include(FindPackageHandleStandardArgs)
include(FetchContent)

# Don't use non-vendored lua; luaL symbols are mandatory.
#find_path(LUA_INCLUDE_DIR lua.h
#    PATH_SUFFIXES lua lua5.4 lua54 lua5.3 lua53
#)
#find_library(LUA_LIBRARY NAMES lua lua5.4 lua54 lua5.3 lua53)
#
#if(LUA_INCLUDE_DIR AND LUA_LIBRARY AND NOT TARGET Lua::Lua)
#    add_library(Lua::Lua UNKNOWN IMPORTED)
#    set_target_properties(Lua::Lua PROPERTIES
#        IMPORTED_LOCATION "${LUA_LIBRARY}"
#        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
#    )
#
#    message("-- Lua Found: ${LUA_INCLUDE_DIR} ${LUA_LIBRARY}")
#endif()

if(NOT TARGET Lua::Lua)
    FetchContent_Declare(lua
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG v5.4.8
        GIT_SHALLOW TRUE
        UPDATE_DISCONNECTED TRUE
    )

    FetchContent_MakeAvailable(lua)

    set(LUA_FETCH_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/lua/include")
    file(MAKE_DIRECTORY "${LUA_FETCH_INCLUDE_DIR}")
    file(COPY
        "${lua_SOURCE_DIR}/lua.h"
        "${lua_SOURCE_DIR}/luaconf.h"
        "${lua_SOURCE_DIR}/lualib.h"
        "${lua_SOURCE_DIR}/lauxlib.h"
        DESTINATION "${LUA_FETCH_INCLUDE_DIR}"
    )
    file(WRITE "${LUA_FETCH_INCLUDE_DIR}/lua.hpp" [=[
#pragma once

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
]=])

    add_library(lua_onelua STATIC "${lua_SOURCE_DIR}/onelua.c")
    target_compile_definitions(lua_onelua PRIVATE MAKE_LIB)
    target_include_directories(lua_onelua PUBLIC "$<BUILD_INTERFACE:${LUA_FETCH_INCLUDE_DIR}>")

    if(APPLE)
        target_compile_definitions(lua_onelua PRIVATE LUA_USE_MACOSX)
    elseif(UNIX)
        target_compile_definitions(lua_onelua PRIVATE LUA_USE_LINUX)
        target_link_libraries(lua_onelua PUBLIC m ${CMAKE_DL_LIBS})
    endif()

    set_target_properties(lua_onelua PROPERTIES
        POSITION_INDEPENDENT_CODE TRUE
    )

    add_library(Lua::Lua ALIAS lua_onelua)
    set(LUA_INCLUDE_DIR "${LUA_FETCH_INCLUDE_DIR}")
    set(LUA_LIBRARY Lua::Lua)
endif()

if(TARGET Lua::Lua)
    set(Lua_FOUND TRUE)
    set(LUA_FOUND TRUE)
    set(LUA_LIBRARIES Lua::Lua)
endif()

find_package_handle_standard_args(Lua DEFAULT_MSG LUA_FOUND)
