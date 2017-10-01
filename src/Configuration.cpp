#include "pch.h"




#define SI_CONVERT_GENERIC
#include "GameState.h"

#include "LuaManager.h"
#include "Logging.h"

using namespace Configuration;

LuaManager *SkinCfgLua;
CSimpleIniA *Config;
int IsWidescreen;
std::string ConfigFile = "config.ini";


const std::string GlobalNamespace = "Global";

class ConfigurationException : std::exception
{
private:
	std::string msg;
public:
	ConfigurationException(std::string what) : exception() { msg = what; }
	const char* what() const noexcept { return msg.c_str(); }
};

ConfigurationException CfgNotLoaded("Configuration not loaded yet.");

void Configuration::SetConfigFile(std::string cfg)
{
    ConfigFile = cfg;
}

void Configuration::Initialize()
{
    Config = new CSimpleIniA;
    Config->LoadFile(ConfigFile.c_str());

    auto dir = Configuration::GetConfigs("GameDirectory");
    if (dir.length()) {
        GameState::GetInstance().SetSystemFolder(dir + "/");
    }

    SkinCfgLua = new LuaManager();

    if (Configuration::GetConfigs("Skin").length())
        GameState::GetInstance().SetSkin(Configuration::GetConfigs("Skin"));

    IsWidescreen = Configuration::GetConfigf("Widescreen");

    SkinCfgLua->SetGlobal("Widescreen", IsWidescreen);
    
	GameState::GetInstance().InitializeLua(SkinCfgLua->GetState());
    SkinCfgLua->RunScript(GameState::GetInstance().GetSkinFile("skin.lua"));

	AddRDLuaGlobal(SkinCfgLua);

    LoadTextureParameters();
}

void Configuration::Cleanup()
{
	if (Config)
		Config->SaveFile(ConfigFile.c_str());

    delete Config;
    delete SkinCfgLua;
}

void Configuration::Reload()
{
	Log::LogPrintf("Reloading configuration...\n");
    delete Config;
    delete SkinCfgLua;
	Initialize();
}

std::string GetConfsInt(std::string Name, std::string Namespace, LuaManager &L)
{
    std::string Retval;
    if (Namespace.length())
    {
        if (L.UseArray(Namespace))
        {
			if (L.CallFunction(Name.c_str(), 0, 1)) {
				if (L.RunFunction())
					Retval = L.GetFunctionResultS();
			}
			else {
				Retval = L.GetFieldS(Name);
			}
			L.Pop();
		}
    }
    else {
		if (L.CallFunction(Name.c_str(), 0, 1)) {
			if (L.RunFunction())
				Retval = L.GetFunctionResultS();
		} else
	        Retval = L.GetGlobalS(Name);
	}
    return Retval;
}

double GetConffInt(std::string Name, std::string Namespace, LuaManager &L)
{
    double Retval = 0;
    if (Namespace.length())
    {
        if (L.UseArray(Namespace))
        {
			if (L.CallFunction(Name.c_str(), 0, 1)) {
				if (L.RunFunction())
					Retval = L.GetFunctionResultD();
			}
			else {
				Retval = L.GetFieldD(Name, 0);
			}

			L.Pop();
        }
    }
    else {
		if (L.CallFunction(Name.c_str(), 0, 1)) {
			if (L.RunFunction())
				Retval = L.GetFunctionResultD();
		} else
	        Retval = L.GetGlobalD(Name, 0);
	}
    return Retval;
}

std::filesystem::path Configuration::GetSkinSound(std::string snd) {
	return GameState::GetInstance().GetSkinFile(GetSkinConfigs(snd, "AudioManifest"));
}

std::string Configuration::GetConfigs(std::string Name, std::string Namespace)
{
    std::string g = GlobalNamespace;
    if (Namespace.length()) g = Namespace;
    std::string out;

	if (!Config) throw CfgNotLoaded;

    if (Config->GetValue(g.c_str(), Name.c_str()))
        out = Config->GetValue(g.c_str(), Name.c_str());
    else
        Config->SetValue(g.c_str(), Name.c_str(), "");
    return out;
}

void Configuration::SetConfig(std::string Name, std::string Value, std::string Namespace)
{
    std::string g = GlobalNamespace;
    if (Namespace.length()) g = Namespace;

	if (!Config) throw CfgNotLoaded;
    Config->SetValue(g.c_str(), Name.c_str(), Value.c_str());
}

float  Configuration::GetConfigf(std::string Name, std::string Namespace)
{
    std::string g = GlobalNamespace;
    double out;
    if (Namespace.length()) g = Namespace;

	if (!Config) throw CfgNotLoaded;
    if (Config->GetDoubleValue(g.c_str(), Name.c_str(), -10000) == -10000)
    {
        Config->SetValue(g.c_str(), Name.c_str(), "0");
        out = 0;
    }
    else
        out = Config->GetDoubleValue(g.c_str(), Name.c_str(), -10000);
    return out;
}

std::string Configuration::GetSkinConfigs(std::string Name, std::string Namespace)
{
    return GetConfsInt(Name, Namespace, *SkinCfgLua);
}

double  Configuration::GetSkinConfigf(std::string Name, std::string Namespace)
{
    return GetConffInt(Name, Namespace, *SkinCfgLua);
}

void Configuration::GetConfigListS(std::string Name, std::map<std::string, std::string> &Out, std::string DefaultKeyName)
{
    CSimpleIniA::TNamesDepend List;
	if (!Config) throw CfgNotLoaded;

    Config->GetAllKeys(Name.c_str(), List);

    if (!List.size() && DefaultKeyName != "")
        Config->SetValue(Name.c_str(), DefaultKeyName.c_str(), "");

    for (CSimpleIniA::TNamesDepend::iterator i = List.begin();
    i != List.end();
    ++i)
    {
        if (Config->GetValue(Name.c_str(), i->pItem))
            Out[std::string(i->pItem)] = Config->GetValue(Name.c_str(), i->pItem);
    }
}

void Configuration::GetConfigListS(std::string Name, std::map<std::string, std::filesystem::path> &Out, std::string DefaultKeyName)
{
    CSimpleIniA::TNamesDepend List;
	if (!Config) throw CfgNotLoaded;

    Config->GetAllKeys(Name.c_str(), List);

    if (!List.size() && DefaultKeyName != "")
        Config->SetValue(Name.c_str(), DefaultKeyName.c_str(), "");

    for (CSimpleIniA::TNamesDepend::iterator i = List.begin();
    i != List.end();
        ++i)
    {
        if (Config->GetValue(Name.c_str(), i->pItem))
            Out[std::string(i->pItem)] = Config->GetValue(Name.c_str(), i->pItem);
    }
}

bool Configuration::ListExists(std::string Name)
{
	if (!Config) throw CfgNotLoaded;

    lua_State *L = SkinCfgLua->GetState();
    bool Exists;

    lua_getglobal(L, Name.c_str());
    Exists = lua_istable(L, -1);

    lua_pop(L, 1);
    return Exists;
}

uint32_t Configuration::CfgScreenHeight()
{
    static uint32_t height = 0;
    if (height == 0) {
        height = GetSkinConfigf("SkinHeight");
        if (height == 0)
            height = ScreenHeightDefault;
    }
	return height;
}

uint32_t Configuration::CfgScreenWidth()
{
    if (IsWidescreen == 1) // 16:9
        return 16.0 / 9.0 * CfgScreenHeight();
    else if (IsWidescreen == 2) // 16:10
        return 16.0 / 10.0 * CfgScreenHeight();
    else 
        return 4.0 / 3.0 * CfgScreenHeight();
}
