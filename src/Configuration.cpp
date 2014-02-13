#include "Global.h"
#include "Configuration.h"
#include "FileManager.h"

#include "LuaManager.h"

#include <iostream>

using namespace Configuration;

LuaManager *CfgLua, *SkinCfgLua;

void Configuration::Initialize()
{
	CfgLua = new LuaManager();
	SkinCfgLua = new LuaManager();

	CfgLua->RunScript("config.lua");

	if (Configuration::GetConfigs("Skin").length())
		FileManager::SetSkin(Configuration::GetConfigs("Skin"));

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
	}else
		Retval = L.GetGlobalS(Name);

	return Retval;
}

float GetConffInt(String Name, String Namespace, LuaManager &L)
{
	float Retval;
	if (Namespace.length())
	{
		L.UseArray(Namespace);
		Retval = L.GetFieldD(Name, 0);
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

float  Configuration::GetSkinConfigf(String Name, String Namespace)
{
	return GetConffInt(Name, Namespace, *SkinCfgLua);
}
