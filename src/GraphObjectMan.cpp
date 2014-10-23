#include "GameGlobal.h"
#include "GameState.h"
#include "GraphObject2D.h"
#include "LuaManager.h"
#include "GraphObjectMan.h"
#include "ImageList.h"

void CreateLuaInterface(LuaManager *AnimLua);

bool LuaAnimation(LuaManager* Lua, String Func, GraphObject2D* Target, float Frac)
{
	Lua->RegisterStruct( "Target", (void*)Target );
	Lua->CallFunction(Func.c_str(), 1, 1);
	Lua->PushArgument(Frac);

	if (Lua->RunFunction())
		return Lua->GetFunctionResult() > 0;
	else
		return false;
}

void GraphObjectMan::StopAnimationsForTarget(GraphObject2D* Target)
{
	for (std::vector<Animation>::iterator i = Animations.begin();
		i != Animations.end();
		)
	{
		if (i->Target == Target)
		{
			i = Animations.erase(i);
			if (i == Animations.end()) break;
		}else
			i++;
	}
}


void GraphObjectMan::AddLuaAnimation (GraphObject2D* Target, const String &FuncName, 
	int Easing, float Duration, float Delay)
{
	Animation Anim;
	Anim.Function = boost::bind(LuaAnimation, Lua, FuncName, _1, _2);
	Anim.Easing = (Animation::EEaseType)Easing;
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

			if (i->Delay < 0) // We rolled into the negatives.
				i->Time += -i->Delay; // Add it to passed time, to pretend it started right on time.
			else
				continue; // It hasn't began yet, so keep at it.
		}else 
			i->Time += TimeDelta;

		if (i->Time >= i->Duration) // The animation is done. Call the function one last time with value 1 so it's completed.
		{
			i->Function (i->Target, 1); 
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
		if (*i == NULL){ /* throw an error */ continue; }
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

void GraphObjectMan::DoEvent(String EventName, int Return)
{
	if (Lua->CallFunction(EventName.c_str(), 0, Return))
		Lua->RunFunction();
}