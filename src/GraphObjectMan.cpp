#include "Global.h"
#include "GraphObject2D.h"
#include "LuaManager.h"
#include "GraphObjectMan.h"

void CreateLuaInterface(LuaManager *AnimLua);

GraphObjectMan::GraphObjectMan()
{
	Lua = new LuaManager;
	Lua->RegisterStruct("GOMAN", this);
	CreateLuaInterface(Lua);
}

GraphObjectMan::~GraphObjectMan()
{
	delete Lua;
}

void GraphObjectMan::Initialize(String Filename)
{
	Lua->RunScript(Filename);

	Lua->CallFunction("Init");
	Lua->RunFunction();
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
	luaL_getmetatable(L, "GraphObject2D");
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
	Lua->CallFunction("Update");
	Lua->PushArgument(TimeDelta);
	Lua->RunFunction();

	for (std::vector<GraphObject2D*>::iterator i = Objects.begin(); i != Objects.end(); i++)
	{
		(*i)->Render();
	}
}

LuaManager *GraphObjectMan::GetEnv()
{
	return Lua;
}