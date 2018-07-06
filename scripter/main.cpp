#include <iostream>
#include <string>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "fs.h"
#include "LuaManager.h"

void define_scripter_api(lua_State* L);

int main(int argc, char const *argv[])
{
    LuaManager lua;

	if (argc < 2) {
		std::cout << "no input supplied" << std::endl;
		return 1;
	}

	define_scripter_api(lua.GetState());

	try {
		std::filesystem::path path = argv[1]; 
		if (!lua.RunScript(path)) {
			std::cerr << "There's been a problem running the script." << std::endl 
				      << lua.GetLastError() << std::endl;
		}
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

    return 0;
}

