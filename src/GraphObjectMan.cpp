#include "GameGlobal.h"
#include "GraphObject2D.h"
#include "LuaManager.h"
#include "GraphObjectMan.h"
#include "FileManager.h"
#include "ImageList.h"

void CreateLuaInterface(LuaManager *AnimLua);

GraphObjectMan::GraphObjectMan()
{
	Lua = new LuaManager;
	Lua->RegisterStruct("GOMAN", this);
	Lua->SetGlobal("ScreenHeight", ScreenHeight);
	Lua->SetGlobal("ScreenWidth", ScreenWidth);
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
			Images->AddToList (Lua->NextString(), FileManager::GetSkinPrefix());
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