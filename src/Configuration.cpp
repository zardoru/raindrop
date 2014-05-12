#include "Global.h"
#include "Configuration.h"
#include "FileManager.h"

#include "LuaManager.h"

#include <iostream>

using namespace Configuration;

LuaManager *CfgLua, *SkinCfgLua;
bool IsWidescreen;

void Configuration::Initialize()
{
	CfgLua = new LuaManager();
	SkinCfgLua = new LuaManager();

	CfgLua->RunScript("config.lua");

	if (Configuration::GetConfigs("Skin").length())
		FileManager::SetSkin(Configuration::GetConfigs("Skin"));

	IsWidescreen = (Configuration::GetConfigf("Widescreen") != 0);

	SkinCfgLua->SetGlobal("Widescreen", IsWidescreen);
	SkinCfgLua->SetGlobal("ScreenWidth", ScreenWidth);
	SkinCfgLua->SetGlobal("ScreenHeight", ScreenHeight);
	SkinCfgLua->RunScript(FileManager::GetSkinPrefix() + "skin.lua");
}

void Configuration::Cleanup()
{
	delete CfgLua;
	delete SkinCfgLua;
}

String GetConfsInt(String Name, String Namespace, LuaManager &L)
{
	String Retval;
	if (Namespace.length())
	{
		L.UseArray(Namespace);
		Retval = L.GetFieldS(Name);
		L.Pop();
	}else
		Retval = L.GetGlobalS(Name);

	return Retval;
}

double GetConffInt(String Name, String Namespace, LuaManager &L)
{
	double Retval;
	if (Namespace.length())
	{
		L.UseArray(Namespace);
		Retval = L.GetFieldD(Name, 0);
		L.Pop();
	}else
		Retval = L.GetGlobalD(Name, 0);

	return Retval;
}

String Configuration::GetConfigs(String Name, String Namespace)
{
	return GetConfsInt(Name, Namespace, *CfgLua);
}

float  Configuration::GetConfigf(String Name, String Namespace)
{
	return GetConffInt(Name, Namespace, *CfgLua);
}

String Configuration::GetSkinConfigs(String Name, String Namespace)
{
	return GetConfsInt(Name, Namespace, *SkinCfgLua);
}

double  Configuration::GetSkinConfigf(String Name, String Namespace)
{
	return GetConffInt(Name, Namespace, *SkinCfgLua);
}

void Configuration::GetConfigListS(String Name, std::vector<String> &Out)
{
	lua_State *L = CfgLua->GetState();

	lua_getglobal(L, Name.c_str());

	if (lua_istable(L, -1))
	{
		lua_pushnil(L);

		while (lua_next(L, -2))
		{
			if (!lua_isnil(L, -1))
			{
				Out.push_back(lua_tostring(L, -1));
			}
			lua_pop(L, 1);
		}
	}
}

double Configuration::CfgScreenHeight()
{
	if (IsWidescreen)
		return ScreenHeightWidescreen;
	else
		return ScreenHeightDefault;
}

double Configuration::CfgScreenWidth()
{
	if (IsWidescreen)
		return ScreenWidthWidescreen;
	else
		return ScreenWidthDefault;
}