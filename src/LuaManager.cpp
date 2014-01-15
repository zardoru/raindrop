#include "Global.h"

#include "LuaManager.h"

int LuaPanic(lua_State* State)
{
	if (lua_isstring(State, -1))
		printf("LUA ERROR: %s\n", lua_tostring(State, -1));
	Utility::DebugBreak();
	return 0;
}

int Break(lua_State *S)
{
	Utility::DebugBreak();
	return 0;
}

LuaManager::LuaManager()
{
	State = luaL_newstate();
	if (State)
	{
		// luaL_openlibs(State);
		RegisterStruct("Luaman", (void*)this);
		Register(Break, "DEBUGBREAK");
		luaL_openlibs(State);
		lua_atpanic(State, &LuaPanic);
	}
	// If we couldn't open lua, can we throw an exception?
}

LuaManager::~LuaManager()
{
	if (State)
		lua_close(State);
}

void LuaManager::GetGlobal(std::string VarName)
{
	lua_getglobal(State, VarName.c_str());
}

bool LuaManager::RunScript(Directory file)
{
	return RunScript(file.path());
}

bool LuaManager::RunScript(std::string Filename)
{
	int errload = 0, errcall = 0;

	if( (errload = luaL_loadfile(State, Filename.c_str())) || (errcall = lua_pcall(State, 0, LUA_MULTRET, 0)))
	{
		std::string reason = lua_tostring(State, -1);

#ifndef NDEBUG
		printf("Lua error: %s\n", reason.c_str());
#endif
		// root->CONSOLE->LogFormat("Error [LuaManager]: Loading file %s\n", Filename.c_str());

		/* if (errload)
			root->CONSOLE->PrintLogFormat("    ->(Loading Error %i: %s)\n", errload, reason.c_str());
		if (errcall)
			root->CONSOLE->PrintLogFormat("    ->(Runtime Error %i: %s)\n", errcall, reason.c_str());
			*/
		return false;
	}
	return true;
}

bool LuaManager::RunString(std::string sString)
{
	int errload = 0, errcall = 0;
	
	if ( (errload = luaL_loadstring(State, sString.c_str())) || (errcall = lua_pcall(State, 0, LUA_MULTRET, 0)) )
	{
		std::string reason = lua_tostring(State, -1);
		/* root->CONSOLE->Print("Error [LuaManager]: Running string.\n");

		if (errload)
			root->CONSOLE->PrintFormat("    ->(Loading Error %i: %s)\n", errload, reason.c_str());
		if (errcall)
			root->CONSOLE->PrintFormat("    ->(Runtime Error %i: %s)\n", errcall, reason.c_str());
			*/
		return false;
	}
	return true;
}

bool LuaManager::IsValid()
{
	return State != NULL;
}

bool LuaManager::Register(lua_CFunction Function, std::string FunctionName)
{
	if (!Function || FunctionName.empty())
		return false;
	lua_register(State, FunctionName.c_str(), Function);
	return true;
}

int LuaManager::GetGlobalI(std::string VariableName, int Default)
{
	int rval = Default;
	
	GetGlobal(VariableName);
	
	if (lua_isnumber(State, -1))
	{
		rval = lua_tonumber(State, -1);
	}else
	{ 
		Pop();
		// throw LuaTypeException(VariableName, "int");
	}

	Pop();
	return rval;
}

std::string LuaManager::GetGlobalS(std::string VariableName, std::string Default)
{
	std::string rval = Default;
	
	GetGlobal(VariableName);
	
	if (lua_isstring(State, -1))
	{
		rval = lua_tostring(State, -1);
	}else
	{
		Pop();
		// throw LuaTypeException(VariableName, "string");
	}

	Pop();
	return rval;
}

double LuaManager::GetGlobalD(std::string VariableName, double Default)
{
	double rval = Default;
	GetGlobal(VariableName);
	if (lua_isnumber(State, -1))
	{
		rval = lua_tonumber(State, -1);
	}else
	{ 
		Pop();
		// throw LuaTypeException(VariableName, "double");
	}

	Pop();
	return rval;
}

void LuaManager::SetGlobal(std::string VariableName, std::string Value)
{
	lua_pushstring(State, Value.c_str());
	lua_setglobal(State, VariableName.c_str());
}

void LuaManager::SetGlobal(std::string VariableName, double Value)
{
	lua_pushnumber(State, Value);
	lua_setglobal(State, VariableName.c_str());
}

bool LuaManager::RegisterStruct(std::string Key, void* data, std::string MetatableName)
{
	if (!data) return false;
	if (Key.length() < 1) return false;

	lua_pushstring(State, Key.c_str());
	lua_pushlightuserdata(State, data);

	if (MetatableName.length())
	{
		luaL_getmetatable(State, MetatableName.c_str());
		lua_setmetatable(State, -2);
	}

	lua_settable(State, LUA_REGISTRYINDEX);
	return true;
}

void* LuaManager::GetStruct(std::string Key)
{
	lua_pushstring(State, Key.c_str());
	lua_gettable(State, LUA_REGISTRYINDEX);
	return lua_touserdata(State, -1); // returns null if does not exist
}

void LuaManager::NewArray()
{
	lua_newtable(State);
}

bool LuaManager::UseArray(std::string VariableName)
{
	GetGlobal(VariableName);
	return lua_istable(State, -1);
}

void LuaManager::SetFieldI(int index, int Value)
{
	lua_pushnumber(State, Value);
	lua_rawseti(State, -2, index);
}

void LuaManager::SetFieldS(int index, std::string Value)
{
	lua_pushstring(State, Value.c_str());
	lua_rawseti(State, -2, index);
}

void LuaManager::SetFieldD(int index, double Value)
{
	lua_pushnumber(State, Value);
	lua_rawseti(State, -2, index);
}

int LuaManager::GetFieldI(std::string Key, int Default)
{
	int R = Default;
	lua_pushstring(State, Key.c_str());
	lua_gettable(State, -2);

	if (lua_isnumber(State, -1))
	{
		R = lua_tonumber(State, -1);
	}// else Error
	
	Pop();
	return R;
}

double LuaManager::GetFieldD(std::string Key, double Default)
{
	double R = Default;

	lua_pushstring(State, Key.c_str());
	lua_gettable(State, -2);

	if (lua_isnumber(State, -1))
	{
		R = lua_tonumber(State, -1);
	}// else Error

	Pop();
	return R;
}

std::string LuaManager::GetFieldS(std::string Key, std::string Default)
{
	std::string R = Default;

	lua_pushstring(State, Key.c_str());
	lua_gettable(State, -2);

	if (lua_isstring(State, -1))
	{
		R = lua_tostring(State, -1);
	}// else Error

	Pop();
	return R;
}

void LuaManager::Pop()
{
	lua_pop(State, 1);
}

void LuaManager::FinalizeArray(std::string ArrayName)
{
	lua_setglobal(State, ArrayName.c_str());
}

void LuaManager::PushArgument(int Value)
{
	if(func_input)
		lua_pushnumber(State, Value);
}

void LuaManager::PushArgument(double Value)
{
	if(func_input)
		lua_pushnumber(State, Value);
}

void LuaManager::PushArgument(std::string Value)
{
	if(func_input)
		lua_pushstring(State, Value.c_str());
}

void LuaManager::CallFunction(std::string Name, int Arguments, int Results)
{	
	func_input = true;
	func_args = Arguments;
	func_results = Results;
	lua_getglobal(State, Name.c_str());
}

bool LuaManager::RunFunction()
{
	func_input = false;
	int errc = lua_pcall(State, func_args, func_results, 0);

	if (errc)
	{
		std::string reason = lua_tostring(State, -1);

#ifndef NDEBUG
		printf("lua call error: %s\n", reason.c_str());
#endif
		return false;
	}

	return true;
}

int LuaManager::GetFunctionResult(int StackPos)
{
	int Value = -1;
	
	if (lua_isnumber(State, -StackPos))
	{
		Value = lua_tonumber(State, -StackPos);

		// Make sure you're getting what you want.
		Pop();
	}

	return Value;
}

void LuaManager::NewMetatable(std::string MtName)
{
	luaL_newmetatable(State, MtName.c_str());
}

void LuaManager::RegisterLibrary(std::string Libname, const luaL_Reg *Reg)
{
	luaL_newlib(State, Reg);
	lua_setglobal(State, Libname.c_str());
}

lua_State* LuaManager::GetState()
{
	return State;
}
