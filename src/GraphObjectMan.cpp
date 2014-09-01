#include "GameGlobal.h"
#include "GameState.h"
#include "GraphObject2D.h"
#include "LuaManager.h"
#include "GraphObjectMan.h"
#include "ImageList.h"

void CreateLuaInterface(LuaManager *AnimLua);

bool LuaAnimation(LuaManager* Lua, const char* Func, GraphObject2D* Target, float Frac)
{
	Lua->RegisterStruct( "Target", (void*)Target );
	Lua->CallFunction(Func, 1, 1);
	Lua->PushArgument(Frac);

	if (Lua->RunFunction())
		return Lua->GetFunctionResult() > 0;
	else
		return false;
}

// Lua Animation stuff: Move these onto their own file eventually.
int LuaAddLuaAnimation(lua_State* L)
{
	GraphObjectMan* GoMan = GetObjectFromState<GraphObjectMan> (L, "GOMAN");
	GraphObject2D* Target = GetObjectFromState<GraphObject2D> (L, "Target");
	String Name = luaL_checkstring(L, 1);
	float Duration = luaL_checknumber(L, 2);
	float Delay = luaL_checknumber(L, 3);
	Animation::EEaseType Easing = (Animation::EEaseType)(int)luaL_checknumber(L, 4);

	GoMan->AddLuaAnimation(Target, Name, Easing, Duration, Delay);
	return 0;
}

void GraphObjectMan::AddLuaAnimation (GraphObject2D* Target, const String &FuncName, 
	Animation::EEaseType Easing, float Duration, float Delay)
{
	Animation Anim;
	Anim.Function = bind(LuaAnimation, Lua, FuncName.c_str(), _1, _2);
	Anim.Easing = Easing;
	Anim.Duration = Duration;
	Anim.Target = Target;
	Anim.Delay = Delay;

	Animations.push_back(Anim);
}

GraphObjectMan::GraphObjectMan()
{
	Animations.reserve(10);
	Lua = new LuaManager;
	Lua->RegisterStruct("GOMAN", this);
	Lua->SetGlobal("ScreenHeight", ScreenHeight);
	Lua->SetGlobal("ScreenWidth", ScreenWidth);

	// Animation constants
	Lua->SetGlobal("EaseNone", Animation::EaseLinear);
	Lua->SetGlobal("EaseIn", Animation::EaseIn);
	Lua->SetGlobal("EaseOut", Animation::EaseOut);
	Lua->Register(LuaAddLuaAnimation, "AddAnimation");

	CreateLuaInterface(Lua);
	Images = new ImageList(true);
}

GraphObjectMan::~GraphObjectMan()
{
	Lua->CallFunction("Cleanup");
	Lua->RunFunction();
	delete Lua;
	delete Images;
}

void GraphObjectMan::Preload(String Filename, String Arrname)
{
	Lua->RunScript(Filename);

	if (Lua->UseArray(Arrname))
	{
		Lua->StartIteration();

		while (Lua->IterateNext())
		{
			Images->AddToList (Lua->NextString(), GameState::GetInstance().GetSkinPrefix());
			Lua->Pop();
		}

		Lua->Pop();
	}
}

void GraphObjectMan::Initialize(String Filename, bool RunScript)
{
	if (RunScript)
		Lua->RunScript(Filename);

	Lua->CallFunction("Init");
	Lua->RunFunction();

	Images->LoadAll();
}

void GraphObjectMan::AddTarget(GraphObject2D *Targ)
{
	Objects.push_back(Targ);
}

void GraphObjectMan::AddLuaTarget(GraphObject2D *Targ, String Varname)
{
	lua_State *L = Lua->GetState();
	GraphObject2D **RetVal = (GraphObject2D**) lua_newuserdata(L, sizeof(GraphObject2D **));
	*RetVal = Targ;
	luaL_getmetatable(L, "Sys.GraphObject2D");
	lua_setmetatable(L, -2);
	lua_setglobal(L, Varname.c_str());
}

void GraphObjectMan::RemoveTarget(GraphObject2D *Targ)
{
	for (std::vector<GraphObject2D*>::iterator i = Objects.begin(); i != Objects.end(); i++)
	{
		if (*i == Targ)
			Objects.erase(i);

		if (i == Objects.end())
			break;
	}
}

void GraphObjectMan::DrawTargets(double TimeDelta)
{
	UpdateTargets(TimeDelta);

	for (std::vector<GraphObject2D*>::iterator i = Objects.begin(); i != Objects.end(); i++)
	{
		(*i)->Render();
	}
}

void GraphObjectMan::UpdateTargets(double TimeDelta)
{
	for (std::vector<Animation>::iterator i = Animations.begin();
		i != Animations.end();
		i++)
	{
		if (i->Delay > 0)
		{
			i->Delay -= TimeDelta; // Still waiting for this to start.
			continue;
		}

		i->Time += TimeDelta;

		if (i->Time >= i->Duration)
		{
			i = Animations.erase(i);
			if (i == Animations.end()) break;
		}

		float frac;

		switch (i->Easing) {
		case Animation::EaseIn:
			frac = pow(i->Time / i->Duration, 2);
			break;
		case Animation::EaseOut:
			frac = i->Time / i->Duration;
			frac = -frac*(frac-2);
			break;
		case Animation::EaseLinear:
		default:
			frac = i->Time / i->Duration;
		}

		if (!i->Function (i->Target, frac)) // Says the animation is over?
		{
			i = Animations.erase(i);
			if (i == Animations.end()) break;
		}
	}

	Lua->CallFunction("Update", 1);
	Lua->PushArgument(TimeDelta);
	Lua->RunFunction();
}

void GraphObjectMan::DrawUntilLayer(uint32 Layer)
{
	for (std::vector<GraphObject2D*>::iterator i = Objects.begin(); i != Objects.end(); i++)
	{
		if ((*i)->GetZ() <= Layer)
			(*i)->Render();
	}
}

void GraphObjectMan::DrawFromLayer(uint32 Layer)
{
	for (std::vector<GraphObject2D*>::iterator i = Objects.begin(); i != Objects.end(); i++)
	{
		if ((*i)->GetZ() >= Layer)
			(*i)->Render();
	}
}


LuaManager *GraphObjectMan::GetEnv()
{
	return Lua;
}

void GraphObjectMan::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	Lua->CallFunction("KeyEvent", 3);
	Lua->PushArgument(key);
	Lua->PushArgument(code);
	Lua->PushArgument(isMouseInput);
	Lua->RunFunction();
}

ImageList* GraphObjectMan::GetImageList()
{
	return Images;
}
