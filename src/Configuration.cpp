#include "Global.h"
#include "Configuration.h"
#include "FileManager.h"

extern "C" {
#include "luaconf.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"


using namespace Configuration;
using namespace luabridge;

lua_State *CfgLua, *SkinCfgLua;

void Configuration::Initialize()
{
	CfgLua = luaL_newstate();
	SkinCfgLua = luaL_newstate();

	luaL_loadfile(CfgLua, "config.lua");
	lua_pcall(CfgLua, 0, LUA_MULTRET, 0);

	luaL_loadfile(SkinCfgLua, (GetConfigs("Skin") + "/skin.lua").c_str());
	lua_pcall(SkinCfgLua, 0, LUA_MULTRET, 0);
}

void Configuration::Cleanup()
{
	lua_close(CfgLua);
	lua_close(SkinCfgLua);
}

String Configuration::GetConfigs(String Name, String Namespace)
{
	if (!Namespace.length())
	{
		LuaRef R = getGlobal(CfgLua, Name.c_str());
		return R.tostring();
	}else
	{
		LuaRef R = getGlobal(CfgLua, Namespace.c_str());
		return std::string(R [Name]);
	}
}

float  Configuration::GetConfigf(String Name, String Namespace)
{
	if (!Namespace.length())
	{
		LuaRef R = getGlobal(CfgLua, Name.c_str());
		return R;
	}else
	{
		LuaRef R = getGlobal(CfgLua, Namespace.c_str());
		return R [Name];
	}
}

String Configuration::GetSkinConfigs(String Name, String Namespace)
{
	if (!Namespace.length())
	{
		LuaRef R = getGlobal(SkinCfgLua, Name.c_str());
		return R.tostring();
	}else
	{
		LuaRef R = getGlobal(SkinCfgLua, Namespace.c_str());
		return std::string(R [Name]);
	}
}

float  Configuration::GetSkinConfigf(String Name, String Namespace)
{
	if (!Namespace.length())
	{
		LuaRef R = getGlobal(SkinCfgLua, Name.c_str());
		return R;
	}else
	{
		LuaRef R = getGlobal(SkinCfgLua, Namespace.c_str());
		return R [Name];
	}
}
