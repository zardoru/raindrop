#include "pch.h"

#include "GameState.h"

#include "LuaManager.h"
#include "Logging.h"

int LuaPanic(lua_State* State)
{
    if (lua_isstring(State, -1))
        wprintf(L"LUA ERROR: %ls\n", Utility::Widen(lua_tostring(State, -1)).c_str());
    Utility::DebugBreak();
    return 0;
}

int Break(lua_State *S)
{
    Utility::DebugBreak();
    return 0;
}

int DoGameScript(lua_State *S)
{
    LuaManager* Lua = GetObjectFromState<LuaManager>(S, "Luaman");
    std::string File = luaL_checkstring(S, 1);
    Lua->Require(GameState::GetInstance().GetScriptsDirectory() + File);
    return 1;
}

LuaManager::LuaManager()
{
    State = luaL_newstate();
    if (State)
    {
        // luaL_openlibs(State);
        RegisterStruct("Luaman", (void*)this);
        Register(Break, "DEBUGBREAK");
        Register(DoGameScript, "game_require");
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

bool LuaManager::RunScript(std::filesystem::path file)
{
    int errload = 0, errcall = 0;

    if (!std::filesystem::exists(file))
        return false;

    Log::LogPrintf("LuaManager: Running script %s.\n", Utility::ToU8(file.wstring()).c_str());

    if ((errload = luaL_loadfile(State, Utility::ToLocaleStr(file).c_str())) || (errcall = lua_pcall(State, 0, LUA_MULTRET, 0)))
    {
        const char* reason = lua_tostring(State, -1);


        if (reason && reason != last_error)
        {
			last_error = reason;
            Log::LogPrintf("LuaManager: Lua error: %s\n", reason);
            Utility::DebugBreak();
        }
        Pop();
        return false;
    }
    return true;
}

bool LuaManager::RunString(std::string string)
{
    int errload = 0, errcall = 0;

    if ((errload = luaL_loadstring(State, string.c_str())) || (errcall = lua_pcall(State, 0, LUA_MULTRET, 0)))
    {
        std::string reason = lua_tostring(State, -1);
        Pop();
        return false;
    }
    return true;
}

bool LuaManager::Require(std::filesystem::path Filename)
{
    lua_getglobal(State, "require");
    lua_pushstring(State, Utility::ToLocaleStr(Filename.wstring()).c_str());
    if (lua_pcall(State, 1, 1, 0))
    {
        const char* reason = lua_tostring(State, -1);
        if (reason && last_error != reason)
        {
			last_error = reason;
            Log::LogPrintf("lua require error: %s\n", reason);
            Utility::DebugBreak();
        }
        // No popping here - if succesful or not we want to leave that return value to lua.
        return false;
    }

    return true;
}

bool LuaManager::IsValid()
{
    return State != nullptr;
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
    }

    Pop();
    return rval;
}

std::string LuaManager::GetGlobalS(std::string VariableName, std::string Default)
{
    std::string rval = Default;

    GetGlobal(VariableName);

    if (!lua_isnil(State, -1) && lua_isstring(State, -1))
    {
        const char* s = lua_tostring(State, -1);
        rval = s ? s : "";
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
    }

    Pop();
    return rval;
}

void LuaManager::SetGlobal(const std::string &VariableName, const std::string &Value)
{
    lua_pushstring(State, Value.c_str());
    lua_setglobal(State, VariableName.c_str());
}

void LuaManager::SetGlobal(const std::string &VariableName, const double &Value)
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
    void* ptr = nullptr;
    lua_pushstring(State, Key.c_str());
    lua_gettable(State, LUA_REGISTRYINDEX);
    ptr = lua_touserdata(State, -1); // returns null if does not exist

    Pop();
    return ptr;
}

void LuaManager::NewArray()
{
    lua_newtable(State);
}

bool LuaManager::UseArray(std::string VariableName)
{
    GetGlobal(VariableName);
    if (lua_istable(State, -1))
        return true;

    Pop();
    return false;
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

void LuaManager::SetFieldS(std::string name, std::string Value)
{
    lua_pushstring(State, Value.c_str());
    lua_setfield(State, -2, name.c_str());
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

    if (lua_istable(State, -1))
    {
        lua_pushstring(State, Key.c_str());
        lua_gettable(State, -2);

        if (lua_isnumber(State, -1))
        {
            R = lua_tonumber(State, -1);
        }// else Error

        Pop();
        return R;
    }
    else
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

void LuaManager::AppendPath(std::string Path)
{
    GetGlobal("package");
    SetFieldS("path", GetFieldS("path") + ";" + Path);
    Pop();
}

void LuaManager::PushArgument(int Value)
{
    if (func_input)
        lua_pushnumber(State, Value);
}

void LuaManager::PushArgument(double Value)
{
    if (func_input)
        lua_pushnumber(State, Value);
}

void LuaManager::PushArgument(std::string Value)
{
    if (func_input)
        lua_pushstring(State, Value.c_str());
}

void LuaManager::PushArgument(bool Value)
{
	if (func_input)
		lua_pushboolean(State, Value);
}

bool LuaManager::CallFunction(const char* Name, int Arguments, int Results)
{
	bool IsFunc;

	func_args = Arguments;
	func_results = Results;

	bool isTable = lua_istable(State, -1);
	if (isTable) {
		lua_pushstring(State, Name);
		lua_gettable(State, -2);
	} else
		lua_getglobal(State, Name);

    IsFunc = lua_isfunction(State, -1);

    if (IsFunc)
        func_input = true;
	else {
		Pop();

		// this is generalizable, but i'm too lazy. -az
		if (isTable) {
			lua_getglobal(State, Name);

			IsFunc = IsFunc = lua_isfunction(State, -1);
			if (IsFunc)
			{
				func_input = true;
			}
			else
				Pop();
		}
	}

    return IsFunc;
}

bool LuaManager::RunFunction()
{
    if (!func_input)
        return false;
    func_input = false;
    int errc = lua_pcall(State, func_args, func_results, 0);

    if (errc)
    {
        std::string reason = lua_tostring(State, -1);

		if (last_error != reason) {
			last_error = reason;
#ifndef WIN32
			printf("lua call error: %s\n", reason.c_str());
#else
			Log::LogPrintf("lua call error: %s\n", reason.c_str());
#endif
		}
        Pop(); // Remove the error from the stack.
        func_err = true;
        return false;
    }

    func_err = false;
    return true;
}

int LuaManager::GetFunctionResult(int StackPos)
{
    return GetFunctionResultD(StackPos);
}

std::string LuaManager::GetFunctionResultS(int StackPos)
{
	std::string Value;

	if (func_err) return Value;

	if (lua_isstring(State, -StackPos))
	{
		Value = lua_tostring(State, -StackPos);
	}

	Pop();
	return Value;
}

float LuaManager::GetFunctionResultF(int StackPos)
{
	return GetFunctionResultD(StackPos);
}

double LuaManager::GetFunctionResultD(int StackPos)
{
	double Value = -1;

	if (func_err) return 0;

	if (lua_isnumber(State, -StackPos))
	{
		Value = lua_tonumber(State, -StackPos);
	}

	Pop();
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

void LuaManager::StartIteration()
{
    lua_pushnil(State);
}

bool LuaManager::IterateNext()
{
    return lua_next(State, -2) != 0;
}

int LuaManager::NextInt()
{
    return lua_tonumber(State, -1);
}

double LuaManager::NextDouble()
{
    return lua_tonumber(State, -1);
}

std::string LuaManager::NextGString()
{
    return lua_tostring(State, -1);
}