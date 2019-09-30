#include <iostream>
#include <string>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "fs.h"
#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

void define_scripter_api(lua_State* L) {
    LuaManager l(L);
    
}