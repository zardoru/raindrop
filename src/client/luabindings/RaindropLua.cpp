#include <TextAndFileUtil.h>
#include <json.hpp>
#include <rmath.h>

#include <game/GameConstants.h>
#include "../PlayscreenParameters.h"
#include "../GameState.h"

#include "Texture.h"
#include "ImageLoader.h"
#include "../LuaManager.h"

#include "../Configuration.h"

namespace LuaAnimFuncs
{
    const char * SpriteMetatable = "Sys.Sprite";

    int GetSkinConfigF(lua_State *L)
    {
        std::string Key = luaL_checkstring(L, 1);
        std::string Namespace = luaL_checkstring(L, 2);

        lua_pushnumber(L, Configuration::GetSkinConfigf(Key, Namespace));
        return 1;
    }

    int GetSkinConfigS(lua_State *L)
    {
        std::string Key = luaL_checkstring(L, 1);
        std::string Namespace = luaL_checkstring(L, 2);

        lua_pushstring(L, Configuration::GetSkinConfigs(Key, Namespace).c_str());
        return 1;
    }

    int Require(lua_State *L)
    {
        auto *Lua = GetObjectFromState<LuaManager>(L, "Luaman");
        std::string Request = luaL_checkstring(L, 1);
        auto File = GameState::GetInstance().GetSkinScriptFile(Request.c_str(), GameState::GetInstance().GetSkin());
        Lua->Require(File);

		// return whatever lua->require left on the stack.
        return 1;
    }

    int FallbackRequire(lua_State *L)
    {
        auto *Lua = GetObjectFromState<LuaManager>(L, "Luaman");
        std::string file = luaL_checkstring(L, 1);
        std::string skin = GameState::GetInstance().GetFirstFallbackSkin();
        if (lua_isstring(L, 2) && lua_tostring(L, 2))
            skin = luaL_checkstring(L, 2);

        if (skin == GameState::GetInstance().GetSkin())
        {
            lua_pushboolean(L, 0);
            return 1;
        }

        Lua->Require(GameState::GetInstance().GetSkinScriptFile(file.c_str(), skin));
        return 1;
    }

    int GetSkinDirectory(lua_State *L)
    {
        lua_pushstring(L, GameState::GetInstance().GetSkinPrefix().c_str());
        return 1;
    }

    int GetSkinFile(lua_State *L)
    {
        auto Out = GameState::GetInstance().GetSkinFile(std::string(luaL_checkstring(L, 1)), GameState::GetInstance().GetSkin());
        lua_pushstring(L, Conversion::ToU8(Out.wstring()).c_str());
        return 1;
    }

    int GetFallbackFile(lua_State *L)
    {
        auto Out = GameState::GetInstance().GetFallbackSkinFile(std::string(luaL_checkstring(L, 1)));
        lua_pushstring(L, Conversion::ToU8(Out.wstring()).c_str());
        return 1;
    }
}

void CreateUtilityLua(LuaManager *AnimLua);
void CreateStringsLuaInterface(LuaManager* AnimLua);
void CreateTransformationLua(LuaManager* anim_lua);
void CreateObject2DLua(LuaManager* anim_lua);
void CreateShaderLua(LuaManager* anim_lua);


void CreateSceneEnvironmentLua(LuaManager* anim_lua);


void DefineSpriteInterface(LuaManager* anim_lua)
{
	AddRDLuaGlobal(anim_lua);

	CreateUtilityLua(anim_lua);
	CreateTransformationLua(anim_lua);
	CreateShaderLua(anim_lua);
	CreateObject2DLua(anim_lua);

	CreateStringsLuaInterface(anim_lua);
}

void AddRDLuaGlobal(LuaManager * anim_lua)
{
	anim_lua->AppendPath("./?;./?.lua");
	anim_lua->AppendPath(GameState::GetInstance().GetScriptsDirectory() + "?");
	anim_lua->AppendPath(GameState::GetInstance().GetScriptsDirectory() + "?.lua");

	anim_lua->AppendPath(GameState::GetInstance().GetSkinPrefix() + "?");
	anim_lua->AppendPath(GameState::GetInstance().GetSkinPrefix() + "?.lua");

	// anim_lua->AppendPath(GameState::GetFallbackSkinPrefix());
	anim_lua->Register(LuaAnimFuncs::Require, "skin_require");
	anim_lua->Register(LuaAnimFuncs::FallbackRequire, "fallback_require");

	// anim_lua->NewMetatable(LuaAnimFuncs::SpriteMetatable);
	anim_lua->Register(LuaAnimFuncs::GetSkinConfigF, "GetConfigF");
	anim_lua->Register(LuaAnimFuncs::GetSkinConfigS, "GetConfigS");
	anim_lua->Register(LuaAnimFuncs::GetSkinDirectory, "GetSkinDirectory");
	anim_lua->Register(LuaAnimFuncs::GetSkinFile, "GetSkinFile");

	anim_lua->NewArray();
	anim_lua->SetFieldI("Height", ScreenHeight);
	anim_lua->SetFieldI("Width", ScreenWidth);
	anim_lua->FinalizeArray("Screen");
}

// New lua interface.
void CreateNewLuaAnimInterface(LuaManager *AnimLua)
{
    DefineSpriteInterface(AnimLua);
	CreateSceneEnvironmentLua(AnimLua);
}

// Old, stateful lua interface.
void CreateLuaInterface(LuaManager *AnimLua)
{
    CreateNewLuaAnimInterface(AnimLua);
}