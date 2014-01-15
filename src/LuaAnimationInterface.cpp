#include "Global.h"

#include "Image.h"
#include "ImageLoader.h"
#include "GraphObject2D.h"
#include "LuaManager.h"
#include "FileManager.h"
#include "GraphObjectMan.h"
#include "Configuration.h"

#ifdef WIN32
#pragma warning (disable : 4244)
#endif

namespace LuaAnimFuncs
{
	const char * GraphObject2DMetatable = "Sys.GraphObject2D";

	int SetRotation(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		float rot = luaL_checknumber(L, 1);
		Target->SetRotation(rot);
		return 0;
	}

	int GetRotation(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		lua_pushnumber(L, Target->GetRotation());
		return 1;
	}

	int Rotate(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		float delta = luaL_checknumber(L, 1);
		Target->AddRotation(delta);
		return 0;
	}

	int Move(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		float x = luaL_checknumber(L, 1), y = luaL_checknumber(L, 2);
		Target->AddPosition( Vec2(x, y) );
		return 0;
	}

	int SetAbsolutePosition(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		Vec2 NewPos(luaL_checknumber(L, 1), luaL_checknumber(L, 2));
		Target->SetPosition(NewPos);
		return 0;
	}

	int GetAbsolutePosition(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		lua_pushnumber(L, Target->GetPosition().x);
		lua_pushnumber(L, Target->GetPosition().y);
		return 2;
	}

	int CropByPixels(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		int x1 = luaL_checknumber(L, 1), y1 = luaL_checknumber(L, 2);
		int x2 = luaL_checknumber(L, 3), y2 = luaL_checknumber(L, 4);
		Target->SetCropByPixels(x1, x2, y1, y2);
		return 0;
	}

	int SetScale(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		float scalex = luaL_checknumber(L, 1), scaley = luaL_checknumber(L, 2);
		Target->SetScale( Vec2( scalex, scaley ) );
		return 0;
	}

	int GetScale(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		lua_pushnumber(L, Target->GetScale().x);
		lua_pushnumber(L, Target->GetScale().y);
		return 2;
	}

	int SetImage(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		std::string iName = luaL_checkstring(L, 1);
		Target->SetImage(ImageLoader::Load(iName));
		return 0;
	}

	int SetImageSkin(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		std::string iName = luaL_checkstring(L, 1);
		Target->SetImage(ImageLoader::LoadSkin(iName));
		return 0;
	}

	int GetSize(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		lua_pushnumber(L, Target->GetWidth());
		lua_pushnumber(L, Target->GetHeight());
		return 2;
	}

	int SetAlpha(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		float alpha = luaL_checknumber(L, 1);
		Target->Alpha = alpha;
		return 0;
	}

	int CreateTarget(lua_State *L)
	{
		GraphObjectMan * Manager = GetObjectFromState<GraphObjectMan>(L, "GOMAN");
		GraphObject2D *Target = new GraphObject2D;
		GraphObject2D **RetVal = (GraphObject2D**) lua_newuserdata(L, sizeof(GraphObject2D **));
		*RetVal = Target;
		luaL_getmetatable(L, GraphObject2DMetatable);
		lua_setmetatable(L, -2);
		Manager->AddTarget(Target);
		return 1;
	}

	int SetTarget(lua_State *L)
	{
		GraphObject2D *Target = *GetUserObject<GraphObject2D*> (L, 1, GraphObject2DMetatable);
		LuaManager *Lua = GetObjectFromState<LuaManager>(L, "Luaman");
		Lua->RegisterStruct("Target", Target);
		return 0;
	}

	int CleanTarget(lua_State *L)
	{
		GraphObject2D *Target = *GetUserObject<GraphObject2D*> (L, 1, GraphObject2DMetatable);
		delete Target;
		return 0;
	}


	int GetSkinConfigF(lua_State *L)
	{
		String Key = luaL_checkstring(L, 1);
		String Namespace = luaL_checkstring(L, 2);

		lua_pushnumber(L, Configuration::GetSkinConfigf(Key, Namespace));
		return 1;
	}

	int GetSkinConfigS(lua_State *L)
	{
		String Key = luaL_checkstring(L, 1);
		String Namespace = luaL_checkstring(L, 2);

		lua_pushstring(L, Configuration::GetSkinConfigs(Key, Namespace).c_str());
		return 1;
	}

	int GetZ(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		lua_pushnumber(L, Target->GetZ());
		return 1;
	}

	int SetZ(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		uint32 Z = luaL_checknumber(L, 1);
		Target->SetZ(Z);
		return 0;
	}

	int SetCentered(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		uint32 Cen = luaL_checknumber(L, 1);
		Target->Centered = (Cen != 0);
		return 0;
	}

	int SetSize(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		float W = luaL_checknumber(L, 1);
		float H = luaL_checknumber(L, 2);
		Target->SetSize(W, H);
		return 0;
	}

	int SetColorInvert(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		uint32 Cen = luaL_checknumber(L, 1);
		Target->ColorInvert = (Cen != 0);
		return 0;
	}

	int SetAffectedbyLightning(lua_State *L)
	{
		GraphObject2D *Target = GetObjectFromState<GraphObject2D>(L, "Target");
		uint32 Cen = luaL_checknumber(L, 1);
		Target->AffectedByLightning = (Cen != 0);
		return 0;
	}

	int Require(lua_State *L)
	{
		LuaManager *Lua = GetObjectFromState<LuaManager>(L, "Luaman");
		lua_pushboolean(L, Lua->RunScript(FileManager::GetSkinPrefix() + luaL_checkstring(L, 1)));
		return 1;
	}

	/*

	int GetGameConfigF(lua_State *L)
	{
	}

	int GetGameConfigS(lua_State *L)
	{
	}

	*/

	static const struct luaL_Reg GraphObjectLib [] = 
	{
		{"SetRotation", SetRotation}, 
		{"GetRotation", GetRotation},
		{"Rotate", Rotate},
		{"Move", Move},
		{"SetPosition", SetAbsolutePosition},
		{"GetPosition", GetAbsolutePosition},
		{"CropByPixels", CropByPixels},
		{"SetScale", SetScale},
		{"GetScale", GetScale},
		{"GetSize", GetSize},
		{"SetSize", SetSize},
		{"SetImage", SetImage},
		{"SetImageSkin", SetImageSkin},
		{"SetAlpha", SetAlpha},
		{"CreateTarget", CreateTarget},
		{"SetTarget", SetTarget},
		{"CleanTarget", CleanTarget},
		{"GetConfigF", GetSkinConfigF},
		{"GetConfigS", GetSkinConfigS},
		{"GetZ", GetZ },
		{"SetZ", SetZ },
		{"SetCentered", SetCentered},
		{"SetColorInvert", SetColorInvert},
		{"SetAffectedbyLightning", SetAffectedbyLightning},
		{NULL, NULL}
	};
}

void CreateLuaInterface(LuaManager *AnimLua)
{
	AnimLua->NewMetatable(LuaAnimFuncs::GraphObject2DMetatable);
	AnimLua->Register(LuaAnimFuncs::Require, "skin_require");
	AnimLua->RegisterLibrary("Obj", ((const luaL_Reg*)LuaAnimFuncs::GraphObjectLib));
}
