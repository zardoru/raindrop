#define SI_CONVERT_GENERIC
#include <memory>
#include <optional>

#include <filesystem>
#include <simpleini/SimpleIni.h>

#include <game/GameConstants.h>
#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"

#include "../game/Game.h"
#include "Configuration.h"

#include "LuaManager.h"
#include "Logging.h"
#include <LuaBridge/LuaBridge.h>

using namespace Configuration;

LuaManager *SkinCfgLua;
std::optional<luabridge::LuaRef> SkinCallbacks;
CSimpleIniA *Config;
int IsWidescreen;
std::string ConfigFile = "config.ini";


const std::string GlobalNamespace = "Global";

class ConfigurationException : public std::exception
{
private:
	std::string msg;
public:
	ConfigurationException(std::string what) : exception() { msg = what; }
	const char* what() const noexcept { return msg.c_str(); }
};

ConfigurationException CfgNotLoaded("Configuration not loaded yet.");

namespace {

bool load_skin_callbacks(const std::filesystem::path &filename)
{
    auto *state = SkinCfgLua->get_lua_state();

    if (!std::filesystem::exists(filename)) {
        Log::LogPrintf("File %s does not exist\n", filename.string().c_str());
        return false;
    }

    if (luaL_loadfile(state, filename.string().c_str())) {
        const char *reason = lua_tostring(state, -1);
        Log::LogPrintf("skin.lua: %s\n", reason ? reason : "unknown error");
        lua_pop(state, 1);
        return false;
    }

    lua_pushcfunction(state, LuaPanic);
    lua_insert(state, -2);

    if (lua_pcall(state, 0, 1, -2)) {
        const char *reason = lua_tostring(state, -1);
        Log::LogPrintf("skin.lua: %s\n", reason ? reason : "unknown error");
        lua_pop(state, 1);
        lua_pop(state, 1);
        return false;
    }

    if (lua_istable(state, -1))
        SkinCallbacks.emplace(luabridge::LuaRef::fromStack(state, -1));
    else
        SkinCallbacks.reset();

    lua_pop(state, 1);
    lua_pop(state, 1);
    return true;
}

template<class LuaValue>
bool is_lua_number(const LuaValue &value)
{
    value.push(value.state());
    const bool result = lua_isnumber(value.state(), -1);
    lua_pop(value.state(), 1);
    return result;
}

template<class LuaValue>
bool is_lua_string(const LuaValue &value)
{
    value.push(value.state());
    const bool result = lua_isstring(value.state(), -1);
    lua_pop(value.state(), 1);
    return result;
}

std::optional<luabridge::LuaRef> get_skin_callback_value(const std::string &name, const std::string &name_space)
{
    if (!SkinCallbacks || !SkinCallbacks->isTable())
        return std::nullopt;

    luabridge::LuaRef value(SkinCallbacks->state(), luabridge::Nil());
    if (name_space.empty()) {
        value = luabridge::LuaRef((*SkinCallbacks)[name]);
    }
    else {
        auto table = luabridge::LuaRef((*SkinCallbacks)[name_space]);
        if (!table.isTable())
            return std::nullopt;

        value = luabridge::LuaRef(table[name]);
    }

    if (value.isFunction()) {
        try {
            value = value();
        }
        catch (const luabridge::LuaException &e) {
            Log::LogPrintf("skin.lua callback error in %s: %s\n", name.c_str(), e.what());
            return std::nullopt;
        }
    }

    if (value.isNil())
        return std::nullopt;

    return value;
}

std::optional<std::string> get_skin_callback_string(const std::string &name, const std::string &name_space)
{
    auto value = get_skin_callback_value(name, name_space);
    if (!value || !is_lua_string(*value))
        return std::nullopt;

    return value->cast<std::string>();
}

std::optional<double> get_skin_callback_number(const std::string &name, const std::string &name_space)
{
    auto value = get_skin_callback_value(name, name_space);
    if (!value || !is_lua_number(*value))
        return std::nullopt;

    return value->cast<double>();
}

}

void Configuration::SetConfigFile(std::string cfg)
{
	Log::LogPrintf("Using configuration file %s\n", cfg.c_str());
    ConfigFile = cfg;
}

void Configuration::Initialize()
{
    Config = new CSimpleIniA;
    Config->LoadFile(ConfigFile.c_str());

    auto dir = Configuration::GetConfigs("GameDirectory");
    if (dir.length()) {
        GameState::get_instance().set_system_folder(dir + "/");
    }

    SkinCfgLua = new LuaManager();

    if (Configuration::GetConfigs("Skin").length())
        GameState::get_instance().set_skin(Configuration::GetConfigs("Skin"));

    IsWidescreen = Configuration::GetConfigf("Widescreen");

    SkinCfgLua->set_global("Widescreen", IsWidescreen);
    
	GameState::get_instance().initialize_lua(SkinCfgLua->get_lua_state());
    load_skin_callbacks(GameState::get_instance().get_skin_file("skin.lua"));

	add_rd_lua_global(SkinCfgLua);

    LoadTextureParameters();
}

void Configuration::cleanup()
{
	if (Config)
		Config->SaveFile(ConfigFile.c_str());

    SkinCallbacks.reset();
    delete Config;
    delete SkinCfgLua;
}

void Configuration::Reload()
{
	Log::LogPrintf("Reloading configuration...\n");
    delete Config;
    SkinCallbacks.reset();
    delete SkinCfgLua;
	Initialize();
}

std::string GetConfsInt(std::string Name, std::string Namespace, LuaManager &L)
{
    if (auto value = get_skin_callback_string(Name, Namespace))
        return *value;

    std::string Retval;
    if (Namespace.length())
    {
        if (L.use_array(Namespace))
        {
			if (L.call_function(Name.c_str(), 0, 1)) {
				if (L.run_function())
					Retval = L.get_function_result_s();
			}
			else {
				Retval = L.get_field_s(Name);
			}
			L.pop();
		}
    }
    else {
		if (L.call_function(Name.c_str(), 0, 1)) {
			if (L.run_function())
				Retval = L.get_function_result_s();
		} else
	        Retval = L.get_global_s(Name);
	}
    return Retval;
}

double GetConffInt(std::string Name, std::string Namespace, LuaManager &L)
{
    if (auto value = get_skin_callback_number(Name, Namespace))
        return *value;

    double Retval = 0;
    if (Namespace.length())
    {
        if (L.use_array(Namespace))
        {
			if (L.call_function(Name.c_str(), 0, 1)) {
				if (L.run_function())
					Retval = L.get_function_result_d();
			}
			else {
				Retval = L.get_field_d(Name, 0);
			}

			L.pop();
        }
    }
    else {
		if (L.call_function(Name.c_str(), 0, 1)) {
			if (L.run_function())
				Retval = L.get_function_result_d();
		} else
	        Retval = L.get_global_d(Name, 0);
	}
    return Retval;
}

std::filesystem::path Configuration::GetSkinSound(std::string snd) {
	return GameState::get_instance().get_skin_file(GetSkinConfigs(snd, "AudioManifest"));
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
	if (SkinCfgLua)
		return GetConffInt(Name, Namespace, *SkinCfgLua);
	else {
		return 0;
	}
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

    lua_State *L = SkinCfgLua->get_lua_state();
    bool Exists;

    if (SkinCallbacks && SkinCallbacks->isTable()) {
        auto list = (*SkinCallbacks)[Name];
        if (list.isTable())
            return true;
    }

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
