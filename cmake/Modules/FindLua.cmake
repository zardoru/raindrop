include(FindPackageHandleStandardArgs)
include(ExternalProject)

find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)

find_path(LUA_INCLUDE_DIR lua.h
    PATH_SUFFIXES lua lua5.4 lua54 lua5.3 lua53
)
find_library(LUA_LIBRARY NAMES lua lua5.4 lua54 lua5.3 lua53)

if(LUA_INCLUDE_DIR AND LUA_LIBRARY AND NOT TARGET Lua::Lua)
    add_library(Lua::Lua UNKNOWN IMPORTED)
    set_target_properties(Lua::Lua PROPERTIES
        IMPORTED_LOCATION "${LUA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
    )
endif()

if(NOT TARGET Lua::Lua)
    set(LUA_EXTERNAL_PREFIX "${CMAKE_BINARY_DIR}/_deps/lua")
    set(LUA_EXTERNAL_INSTALL_DIR "${LUA_EXTERNAL_PREFIX}/install")
    set(LUA_EXTERNAL_INCLUDE_DIR "${LUA_EXTERNAL_INSTALL_DIR}/include")
    set(LUA_EXTERNAL_LIBRARY "${LUA_EXTERNAL_INSTALL_DIR}/lib/liblua.a")
    set(LUA_EXTERNAL_HPP "${LUA_EXTERNAL_PREFIX}/lua.hpp")

    file(MAKE_DIRECTORY "${LUA_EXTERNAL_INCLUDE_DIR}")
    file(WRITE "${LUA_EXTERNAL_HPP}" [=[
#pragma once

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
]=])

    if(APPLE)
        set(LUA_EXTERNAL_CFLAGS "-DLUA_USE_MACOSX -fPIC")
    elseif(UNIX)
        set(LUA_EXTERNAL_CFLAGS "-DLUA_USE_LINUX -fPIC")
    else()
        set(LUA_EXTERNAL_CFLAGS "-fPIC")
    endif()

    ExternalProject_Add(lua_external
        PREFIX "${LUA_EXTERNAL_PREFIX}"
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG v5.4.8
        GIT_SHALLOW TRUE
        UPDATE_COMMAND ""
        UPDATE_DISCONNECTED TRUE
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE TRUE
        BUILD_BYPRODUCTS "${LUA_EXTERNAL_LIBRARY}"
        BUILD_COMMAND
            "${MAKE_EXECUTABLE}"
            "CC=${CMAKE_C_COMPILER}"
            "MYCFLAGS=${LUA_EXTERNAL_CFLAGS}"
            "MYLDFLAGS="
            "MYLIBS="
            a
        INSTALL_COMMAND
            "${CMAKE_COMMAND}" -E make_directory "${LUA_EXTERNAL_INSTALL_DIR}/lib"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${LUA_EXTERNAL_INCLUDE_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "<SOURCE_DIR>/liblua.a" "${LUA_EXTERNAL_LIBRARY}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "<SOURCE_DIR>/lua.h"
                "<SOURCE_DIR>/luaconf.h"
                "<SOURCE_DIR>/lualib.h"
                "<SOURCE_DIR>/lauxlib.h"
                "${LUA_EXTERNAL_HPP}"
                "${LUA_EXTERNAL_INCLUDE_DIR}"
    )

    add_library(Lua::Lua STATIC IMPORTED)
    set_target_properties(Lua::Lua PROPERTIES
        IMPORTED_LOCATION "${LUA_EXTERNAL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUA_EXTERNAL_INCLUDE_DIR}"
    )
    add_dependencies(Lua::Lua lua_external)
    set(LUA_INCLUDE_DIR "${LUA_EXTERNAL_INCLUDE_DIR}")
    set(LUA_LIBRARY Lua::Lua)
endif()

if(TARGET Lua::Lua)
    set(Lua_FOUND TRUE)
    set(LUA_FOUND TRUE)
    set(LUA_LIBRARIES Lua::Lua)
endif()

find_package_handle_standard_args(Lua DEFAULT_MSG LUA_FOUND)
