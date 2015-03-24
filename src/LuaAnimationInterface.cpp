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

	int SetRotation(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		double rot = luaL_checknumber(L, 1);
		Target->SetRotation(rot);
		return 0;
	}

	int GetRotation(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		lua_pushnumber(L, Target->GetRotation());
		return 1;
	}

	int Rotate(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		double delta = luaL_checknumber(L, 1);
		Target->AddRotation(delta);
		return 0;
	}

	int Move(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		double x = luaL_checknumber(L, 1), y = luaL_checknumber(L, 2);
		Target->AddPosition(Vec2(x, y));
		return 0;
	}

	int SetAbsolutePosition(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		Vec2 NewPos(luaL_checknumber(L, 1), luaL_checknumber(L, 2));
		Target->SetPosition(NewPos);
		return 0;
	}

	int GetAbsolutePosition(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		lua_pushnumber(L, Target->GetPosition().x);
		lua_pushnumber(L, Target->GetPosition().y);
		return 2;
	}

	int CropByPixels(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		int x1 = luaL_checknumber(L, 1), y1 = luaL_checknumber(L, 2);
		int x2 = luaL_checknumber(L, 3), y2 = luaL_checknumber(L, 4);
		Target->SetCropByPixels(x1, x2, y1, y2);
		return 0;
	}

	int SetScale(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		double scalex = luaL_checknumber(L, 1), scaley = luaL_checknumber(L, 2);
		Target->SetScale(Vec2(scalex, scaley));
		return 0;
	}

	int GetScale(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		lua_pushnumber(L, Target->GetScale().x);
		lua_pushnumber(L, Target->GetScale().y);
		return 2;
	}

	int SetImage(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		GString iName = luaL_checkstring(L, 1);
		Target->SetImage(ImageLoader::Load(iName));
		return 0;
	}

	int SetImageSkin(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		GString iName = luaL_checkstring(L, 1);
		Target->SetImage(GameState::GetInstance().GetSkinImage(iName));
		return 0;
	}

	int GetSize(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		lua_pushnumber(L, Target->GetWidth());
		lua_pushnumber(L, Target->GetHeight());
		return 2;
	}

	int SetAlpha(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		double alpha = luaL_checknumber(L, 1);
		Target->Alpha = alpha;
		return 0;
	}

	int GetAlpha(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		lua_pushnumber(L, Target->Alpha);
		return 1;
	}

	int CreateTarget(lua_State *L)
	{
		SceneEnvironment * Manager = GetObjectFromState<SceneEnvironment>(L, "GOMAN");
		Sprite *Target = Manager->CreateObject();
		Sprite **RetVal = (Sprite**)lua_newuserdata(L, sizeof(Sprite **));
		*RetVal = Target;
		luaL_getmetatable(L, SpriteMetatable);
		lua_setmetatable(L, -2);
		return 1;
	}

	int SetTarget(lua_State *L)
	{
		Sprite *Target = *GetUserObject<Sprite*>(L, 1, SpriteMetatable);
		LuaManager *Lua = GetObjectFromState<LuaManager>(L, "Luaman");
		Lua->RegisterStruct("Target", Target);
		return 0;
	}

	int CleanTarget(lua_State *L)
	{
		Sprite *Target = *GetUserObject<Sprite*>(L, 1, SpriteMetatable);
		SceneEnvironment * Manager = GetObjectFromState<SceneEnvironment>(L, "GOMAN");

		Manager->RemoveManagedObject(Target);
		return 0;
	}


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

	int GetZ(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		lua_pushnumber(L, Target->GetZ());
		return 1;
	}

	int SetZ(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		uint32 Z = luaL_checknumber(L, 1);
		Target->SetZ(Z);
		return 0;
	}

	int SetCentered(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		uint32 Cen = luaL_checknumber(L, 1);
		Target->Centered = (Cen != 0);
		return 0;
	}

	int SetSize(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		double W = luaL_checknumber(L, 1);
		double H = luaL_checknumber(L, 2);
		Target->SetSize(W, H);
		return 0;
	}

	int SetColorInvert(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		uint32 Cen = luaL_checknumber(L, 1);
		Target->ColorInvert = (Cen != 0);
		return 0;
	}

	int SetAffectedbyLightning(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		uint32 Cen = luaL_checknumber(L, 1);
		Target->AffectedByLightning = (Cen != 0);
		return 0;
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

	int SetColor(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		double R = luaL_checknumber(L, 1), G = luaL_checknumber(L, 2), B = luaL_checknumber(L, 3);

		Target->Red = R;
		Target->Green = G;
		Target->Blue = B;
		return 0;
	}

	int SetLighten(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		int Lighten = luaL_checkinteger(L, 1);
		Target->Lighten = (Lighten != 0);
		return 0;
	}

	int SetLightenFactor(lua_State *L)
	{
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");
		float LightenFactor = luaL_checknumber(L, 1);
		Target->LightenFactor = LightenFactor;
		return 0;
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

	int SetBlendMode(lua_State *L)
	{
		int B = luaL_checknumber(L, 1);
		Sprite *Target = GetObjectFromState<Sprite>(L, "Target");

		Target->SetBlendMode((RBlendMode)B);
		return 0;
	}

	// Lua Animation stuff: Move these onto their own file eventually.
	int AddLuaAnimation(lua_State* L)
	{
		SceneEnvironment* GoMan = GetObjectFromState<SceneEnvironment> (L, "GOMAN");
		Sprite* Target = GetObjectFromState<Sprite> (L, "Target");
		GString Name = luaL_checkstring(L, 1);
		float Duration = luaL_checknumber(L, 2);
		float Delay = luaL_checknumber(L, 3);
		Animation::EEaseType Easing = (Animation::EEaseType)(int)luaL_checknumber(L, 4);

		GoMan->AddLuaAnimation(Target, Name, Easing, Duration, Delay);
		return 0;
	}

	int LuaStopAnimationsForTarget(lua_State *L)
	{
		SceneEnvironment* GoMan = GetObjectFromState<SceneEnvironment> (L, "GOMAN");
		Sprite* Target = GetObjectFromState<Sprite> (L, "Target");

		GoMan->StopAnimationsForTarget(Target);
		return 0;
	}

	int SetUILayer(lua_State *L)
	{
		uint32 Layer = luaL_checkinteger(L, 1);
		SceneEnvironment* GoMan = GetObjectFromState<SceneEnvironment>(L, "GOMAN");
		GoMan->SetUILayer(Layer);
		return 0;
	}

	static const struct luaL_Reg GraphObjectLib[] =
	{
		{ "SetRotation", SetRotation },
		{ "GetRotation", GetRotation },
		{ "Rotate", Rotate },
		{ "Move", Move },
		{ "SetColor", SetColor },
		{ "SetPosition", SetAbsolutePosition },
		{ "GetPosition", GetAbsolutePosition },
		{ "CropByPixels", CropByPixels },
		{ "SetScale", SetScale },
		{ "GetScale", GetScale },
		{ "GetSize", GetSize },
		{ "SetSize", SetSize },
		{ "SetImage", SetImage },
		{ "SetImageSkin", SetImageSkin },
		{ "SetAlpha", SetAlpha },
		{ "GetAlpha", GetAlpha },
		{ "CreateTarget", CreateTarget },
		{ "SetTarget", SetTarget },
		{ "CleanTarget", CleanTarget },
		{ "SetBlendMode", SetBlendMode },
		{ "GetZ", GetZ },
		{ "SetZ", SetZ },
		{ "SetCentered", SetCentered },
		{ "SetColorInvert", SetColorInvert },
		{ "SetAffectedbyLightning", SetAffectedbyLightning },
		{ "GetSkinDirectory", GetSkinDirectory },
		{ "GetSkinFile", GetSkinFile },
		{ "AddAnimation", AddLuaAnimation },
		{ "ClearAnimations", LuaStopAnimationsForTarget },
		{ "SetLighten", SetLighten },
		{ "SetLightenFactor", SetLightenFactor },
		{ "SetUILayer", SetUILayer },
		{ NULL, NULL }
	};
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
	AnimLua->RegisterLibrary("Obj", ((const luaL_Reg*)LuaAnimFuncs::GraphObjectLib));
	
	AnimLua->SetGlobal("ScreenHeight", ScreenHeight);
	AnimLua->SetGlobal("ScreenWidth", ScreenWidth);

	// Animation constants
	AnimLua->SetGlobal("EaseNone", Animation::EaseLinear);
	AnimLua->SetGlobal("EaseIn", Animation::EaseIn);
	AnimLua->SetGlobal("EaseOut", Animation::EaseOut);

	AnimLua->SetGlobal("BlendAdd", (int)MODE_ADD);
	AnimLua->SetGlobal("BlendAlpha", (int)MODE_ALPHA);
}
