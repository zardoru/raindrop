#include "Global.h"
#include "GameState.h"

#include "Image.h"
#include "ImageLoader.h"
#include "Sprite.h"
#include "LuaManager.h"
#include "SceneEnvironment.h"
#include "Configuration.h"
#include "LuaBridge.h"

#include "Font.h"
#include "TruetypeFont.h"
#include "BitmapFont.h"
#include "Line.h"

#include "GraphicalString.h"

namespace LuaAnimFuncs
{
	const char * SpriteMetatable = "Sys.Sprite";

	int GetSkinConfigF(lua_State *L)
	{
		GString Key = luaL_checkstring(L, 1);
		GString Namespace = luaL_checkstring(L, 2);

		lua_pushnumber(L, Configuration::GetSkinConfigf(Key, Namespace));
		return 1;
	}

	int GetSkinConfigS(lua_State *L)
	{
		GString Key = luaL_checkstring(L, 1);
		GString Namespace = luaL_checkstring(L, 2);

		lua_pushstring(L, Configuration::GetSkinConfigs(Key, Namespace).c_str());
		return 1;
	}

	int Require(lua_State *L)
	{
		LuaManager *Lua = GetObjectFromState<LuaManager>(L, "Luaman");
		lua_pushboolean(L, Lua->RunScript(GameState::GetInstance().GetSkinFile(luaL_checkstring(L, 1))));
		return 1;
	}

	int FallbackRequire(lua_State *L)
	{
		LuaManager *Lua = GetObjectFromState<LuaManager>(L, "Luaman");
		lua_pushboolean(L, Lua->RunScript(GameState::GetInstance().GetFallbackSkinFile(luaL_checkstring(L, 1))));
		return 1;
	}

	int GetSkinDirectory(lua_State *L)
	{
		lua_pushstring(L, GameState::GetInstance().GetSkinPrefix().c_str());
		return 1;
	}

	int GetSkinFile(lua_State *L)
	{
		GString Out = GameState::GetInstance().GetSkinFile(GString(luaL_checkstring(L, 1)));
		lua_pushstring(L, Out.c_str());
		return 1;
	}

	int GetFallbackFile(lua_State *L)
	{
		GString Out = GameState::GetInstance().GetFallbackSkinFile(GString(luaL_checkstring(L, 1)));
		lua_pushstring(L, Out.c_str());
		return 1;
	}
}

// Wrapper functions
void SetImage(Sprite *O, GString dir)
{
	O->SetImage(GameState::GetInstance().GetSkinImage(dir));
}

GString GetImage(const Sprite *O)
{
	return O->GetImageFilename();
}

void AddPosition(Sprite *O, float px, float py)
{
	O->AddPosition(px, py);
}

void SetPosition(Sprite *O, float px, float py)
{
	O->SetPosition(px, py);
}

void LoadBmFont(BitmapFont* B, GString Fn, float CellWidth, float CellHeight, float CharWidth, float CharHeight, int startChar)
{
	Vec2 Size(CharWidth, CharHeight);
	Vec2 CellSize(CellWidth, CellHeight);
	B->LoadSkinFontImage(Fn.c_str(), Size, CellSize, Size, startChar);
}

// We need these to be able to work with gcc.
// Adding these directly does not work. Inheriting them from Transformation does not work. We're left only with this.
struct O2DProxy
{
	static uint32 getZ(Sprite const* obj)
	{
		return obj->GetZ();
	}

	static float getScaleX(Sprite const* obj)
	{
		return obj->GetScaleX();
	}

	static float getScaleY(Sprite const* obj)
	{
		return obj->GetScaleY();
	}

	static float getWidth(Sprite const* obj)
	{
		return obj->GetWidth();
	}

	static float getHeight(Sprite const* obj)
	{
		return obj->GetHeight();
	}

	static float getX(Sprite const* obj)
	{
		return obj->GetPositionX();
	}

	static float getY(Sprite const* obj)
	{
		return obj->GetPositionY();
	}

	static float getRotation(Sprite const* obj)
	{
		return obj->GetRotation();
	}

	static Transformation getChainTransformation(Sprite const* obj)
	{
		return Transformation();
	}

	static void setZ(Sprite *obj, uint32 nZ)
	{
		obj->SetZ(nZ);
	}

	static void setHeight(Sprite *obj, float param)
	{
		obj->SetHeight(param);
	}

	static void setWidth(Sprite *obj, float param)
	{
		obj->SetWidth(param);
	}

	static void setScaleY(Sprite *obj, float param)
	{
		obj->SetScaleY(param);
	}

	static void setScaleX(Sprite *obj, float param)
	{
		obj->SetScaleX(param);
	}

	static void setRotation(Sprite *obj, float param)
	{
		obj->SetRotation(param);
	}

	static void setX(Sprite *obj, float param)
	{
		obj->SetPositionX(param);
	}

	static void setY(Sprite *obj, float param)
	{
		obj->SetPositionY(param);
	}

	static void setChainTransformation(Sprite *obj, Transformation* param)
	{
		obj->ChainTransformation(param);
	}

	static void setScale(Sprite *obj, float param)
	{
		obj->SetScale(param);
	}

	static void AddRotation(Sprite* obj, float param)
	{
		obj->AddRotation(param);
	}
};

// New lua interface.
void CreateNewLuaAnimInterface(LuaManager *AnimLua)
{
#define f(x) addFunction(#x, &Sprite::x)
#define p(x) addProperty(#x, &Sprite::Get##x, &Sprite::Set##x)
#define v(x) addData(#x, &Sprite::x)
#define q(x) addProperty(#x, &O2DProxy::get##x, &O2DProxy::set##x)

	luabridge::getGlobalNamespace(AnimLua->GetState())
		.beginClass <Transformation>("Transformation")
		.addConstructor<void(*) ()>()
		.addProperty("Z", &Transformation::GetZ, &Transformation::SetZ)
		.addProperty("Layer", &Transformation::GetZ, &Transformation::SetZ)
		.addProperty("Rotation", &Transformation::GetRotation, &Transformation::SetRotation)
		.addProperty("Width", &Transformation::GetWidth, &Transformation::SetWidth)
		.addProperty("Height", &Transformation::GetHeight, &Transformation::SetHeight)
		.addProperty("ScaleX", &Transformation::GetScaleX, &Transformation::SetScaleX)
		.addProperty("ScaleY", &Transformation::GetScaleY, &Transformation::SetScaleY)
		.addProperty("X", &Transformation::GetPositionX, &Transformation::SetPositionX)
		.addProperty("Y", &Transformation::GetPositionY, &Transformation::SetPositionY)
		.addFunction("SetChainTransformation", &Transformation::ChainTransformation)
		.endClass();

	luabridge::getGlobalNamespace(AnimLua->GetState())
		.deriveClass<Sprite, Transformation>("Object2D")
		.v(Centered)
		.v(Lighten)
		.v(LightenFactor)
		.v(AffectedByLightning)
		.v(ColorInvert)
		.v(Alpha)
		.v(Red)
		.v(Blue)
		.v(Green)
		.p(BlendMode)
		.f(SetCropByPixels)
		.addFunction<void (Sprite::*)(float)>("AddRotation", &Sprite::AddRotation)
		.addFunction<void (Sprite::*)(float)>("SetScale", &Sprite::SetScale)
		.q(Z)
		.addProperty("Layer", &O2DProxy::getZ, &O2DProxy::setZ)
		.q(ScaleX)
		.q(ScaleY)
		.q(Rotation)
		.q(Width)
		.q(Height)
		.q(X)
		.q(Y)
		.q(ChainTransformation)
		.addProperty("Image", GetImage, SetImage) // Special for setting image.
		.endClass();
	
	/*
	luabridge::getGlobalNamespace(AnimLua->GetState())
		.beginClass <Line>("Line2D")
		.addConstructor<void(*) ()>()
		.addFunction("SetColor", &Line::SetColor)
		.addFunction("SetLocation", &Line::SetLocation)
		.endClass();
		*/

	luabridge::getGlobalNamespace(AnimLua->GetState())
		.beginClass <SceneEnvironment> ("GraphObjMan")
		.addFunction("AddAnimation", &SceneEnvironment::AddLuaAnimation)
		.addFunction("AddTarget", &SceneEnvironment::AddTarget)
		.addFunction("Sort", &SceneEnvironment::Sort)
		.addFunction("StopAnimation", &SceneEnvironment::StopAnimationsForTarget)
		.addFunction("SetUILayer", &SceneEnvironment::SetUILayer)
		.addFunction("RunUIScript", &SceneEnvironment::RunUIScript)
		.addFunction("CreateObject", &SceneEnvironment::CreateObject)
		.endClass();

	luabridge::push(AnimLua->GetState(), GetObjectFromState<SceneEnvironment>(AnimLua->GetState(), "GOMAN"));
	lua_setglobal(AnimLua->GetState(), "Engine");
#undef f
#undef p
#undef v

	// These are mostly defined so we can pass them around and construct them.
	luabridge::getGlobalNamespace(AnimLua->GetState())
		.beginNamespace("Fonts")
		.beginClass<Font>("Font")
		.addFunction("SetColor", &Font::SetColor)
		.addFunction("SetAlpha", &Font::SetAlpha)
		.addFunction("GetLength", &Font::GetHorizontalLength)
		.endClass()
		.deriveClass <TruetypeFont, Font>("TruetypeFont")
		.addConstructor <void(*) (GString, float)>()
		.endClass()
		.deriveClass <BitmapFont, Font> ("BitmapFont")
		.addConstructor<void(*)()>()
		.endClass()
		.addFunction("LoadBitmapFont", LoadBmFont)
		.endNamespace();

	luabridge::getGlobalNamespace(AnimLua->GetState())
		.deriveClass<GraphicalString, Sprite>("StringObject2D")
		.addConstructor <void(*) ()>()
		.addProperty("Font", &GraphicalString::GetFont, &GraphicalString::SetFont)
		.addProperty ("Text", &GraphicalString::GetText, &GraphicalString::SetText)
		.endClass();
}

// Old, stateful lua interface.
void CreateLuaInterface(LuaManager *AnimLua)
{
	CreateNewLuaAnimInterface(AnimLua);

	AnimLua->NewMetatable(LuaAnimFuncs::SpriteMetatable);
	AnimLua->Register(LuaAnimFuncs::Require, "skin_require");
	AnimLua->Register(LuaAnimFuncs::FallbackRequire, "fallback_require");
	AnimLua->Register(LuaAnimFuncs::GetSkinConfigF, "GetConfigF");
	AnimLua->Register(LuaAnimFuncs::GetSkinConfigS, "GetConfigS");
	AnimLua->Register(LuaAnimFuncs::GetSkinDirectory, "GetSkinDirectory");
	AnimLua->Register(LuaAnimFuncs::GetSkinFile, "GetSkinFile");
	
	AnimLua->SetGlobal("ScreenHeight", ScreenHeight);
	AnimLua->SetGlobal("ScreenWidth", ScreenWidth);

	// Animation constants
	AnimLua->SetGlobal("EaseNone", Animation::EaseLinear);
	AnimLua->SetGlobal("EaseIn", Animation::EaseIn);
	AnimLua->SetGlobal("EaseOut", Animation::EaseOut);

	AnimLua->SetGlobal("BlendAdd", (int)MODE_ADD);
	AnimLua->SetGlobal("BlendAlpha", (int)MODE_ALPHA);
}
