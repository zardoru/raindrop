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

	if (Configuration::GetConfigs("Skin").length())
		FileManager::SetSkin(Configuration::GetConfigs("Skin"));

	luaL_loadfile(SkinCfgLua, (FileManager::GetSkinPrefix() + "/skin.lua").c_str());
	if (lua_pcall(SkinCfgLua, 0, LUA_MULTRET, 0))
	{
		std::string reason = lua_tostring(SkinCfgLua, -1);
		Utility::DebugBreak();
	}
}

void Configuration::Cleanup()
{
	lua_close(CfgLua);
	lua_close(SkinCfgLua);
}

String GetConfsInt(String Name, String Namespace, lua_State *L)
{
	if (!Namespace.length())
	{
		LuaRef R = getGlobal(L, Name.c_str());
		if (!R.isNil())
			return R;
		else
			return "";
	}else
	{
		LuaRef R = getGlobal(L, Namespace.c_str());
		if (!R.isNil() && !R[Name].isNil())
			return std::string(R [Name]);
		else
			return "";

	}
}

float GetConffInt(String Name, String Namespace, lua_State *L)
{
	if (!Namespace.length())
	{
		LuaRef R = getGlobal(L, Name.c_str());
		if (!R.isNil())
			return R;
		else
			return 0;
	}else
	{
		LuaRef R = getGlobal(L, Namespace.c_str());
		if (!R.isNil() && !R[Name].isNil())
			return R [Name];
		else
			return 0;

	}
}

String Configuration::GetConfigs(String Name, String Namespace)
{
	return GetConfsInt(Name, Namespace, CfgLua);
}

float  Configuration::GetConfigf(String Name, String Namespace)
{
	return GetConffInt(Name, Namespace, CfgLua);
}

String Configuration::GetSkinConfigs(String Name, String Namespace)
{
	return GetConfsInt(Name, Namespace, SkinCfgLua);
}

float  Configuration::GetSkinConfigf(String Name, String Namespace)
{
	return GetConffInt(Name, Namespace, SkinCfgLua);
}
