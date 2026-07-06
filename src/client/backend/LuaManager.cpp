#include <string>
#include <filesystem>
#include <vector>

#include "LuaManager.h"
#include "Logging.h"
#include <text_and_file_util.h>
#include <LuaBridge/LuaBridge.h>

namespace {

template<class LuaValue>
bool is_lua_number(const LuaValue &value)
{
    value.push(value.state());
    const bool result = lua_isnumber(value.state(), -1);
    lua_pop(value.state(), 1);
    return result;
}

template<class LuaValue>
bool is_lua_string(const LuaValue &value)
{
    value.push(value.state());
    const bool result = lua_isstring(value.state(), -1);
    lua_pop(value.state(), 1);
    return result;
}

template<class LuaValue>
int to_int_or_default(const LuaValue &value, const int default_value)
{
    return is_lua_number(value) ? static_cast<int>(value.template cast<double>()) : default_value;
}

template<class LuaValue>
double to_double_or_default(const LuaValue &value, const double default_value)
{
    return is_lua_number(value) ? value.template cast<double>() : default_value;
}

template<class LuaValue>
std::string to_string_or_default(const LuaValue &value, const std::string &default_value)
{
    return is_lua_string(value) ? value.template cast<std::string>() : default_value;
}

}

int LuaPanic(lua_State* State)
{
	const char* msg = nullptr;
	if (lua_isstring(State, 1)) {
		msg = lua_tostring(State, 1);
		luaL_traceback(State, State, msg, 2);
		return 1;
	}
	else {
		return 0;
	}
}

int Break(lua_State *S)
{
    otoworm::util::debug_break();
    return 0;
}


int LuaReadOnlyError(lua_State *S)
{
	luaL_error(S, "tried to write to read-only table");
	return 0;
}

LuaManager::LuaManager()
{
	WeOwnThisState = true;
    State = luaL_newstate();
    if (State)
    {
        // luaL_openlibs(State);
        register_struct("Luaman", (void*)this);
        register_function(Break, "DEBUGBREAK");
        luaL_openlibs(State);
        lua_atpanic(State, &LuaPanic);
    }
    // If we couldn't open lua, can we throw an exception?

	func_input = func_args = func_results = 0;
	func_err = false;
}

LuaManager::LuaManager(lua_State *L)
{
	WeOwnThisState = false;
	State = L;
}

LuaManager::~LuaManager()
{
    if (State && WeOwnThisState)
        lua_close(State);
}

void LuaManager::get_global(std::string VarName)
{
    lua_getglobal(State, VarName.c_str());
}

void reportError(lua_State *State)
{
	const char* reason = lua_tostring(State, -1);


    // Log::LogPrintf("LuaManager: Lua error: %s\n", reason);
#ifndef TESTS
    otoworm::util::debug_break();
#endif
    lua_pop(State, 1);
}

std::string LuaManager::get_last_error()
{
    return last_error;
}

bool LuaManager::run_script(std::filesystem::path file)
{
    int errload = 0, errcall = 0;

    if (!std::filesystem::exists(file))
    {
        last_error = otoworm::util::format("File %s does not exist", file.string().c_str());
        return false;    
    }

	if ((errload = luaL_loadfile(State, file.string().c_str()))) {
		std::string s = lua_tostring(State, -1);
        last_error = s;
        pop();
		return false;
	}
	
	lua_pushcfunction(State, LuaPanic);
	lua_insert(State, -2);

	if ((errcall = lua_pcall(State, 0, 0, -2)))
    {
		std::string s = lua_tostring(State, -1);
        last_error = s;
        pop(); // remove error
		pop(); // remove pushed panic function
		return false;
    }

	pop(); // remove panic func.
    return true;
}

bool LuaManager::run_string(std::string string)
{
    int errload = 0, errcall = 0;

    if ((errload = luaL_loadstring(State, string.c_str())) || (errcall = lua_pcall(State, 0, LUA_MULTRET, 0)))
    {
        std::string reason = lua_tostring(State, -1);
        last_error = reason;
        pop();
        return false;
    }
    return true;
}

bool LuaManager::require(std::filesystem::path Filename)
{
	lua_pushcfunction(State, LuaPanic);
    lua_getglobal(State, "require");
    lua_pushstring(State, otoworm::locale::to_locale_str(Filename.wstring()).c_str());
    if (lua_pcall(State, 1, 1, -3))
    {
        const char* reason = lua_tostring(State, -1);
        if (reason && last_error != reason)
        {
			last_error = reason;
        }

		lua_remove(State, -2); // Remove the traceback.
        // No popping here - if succesful or not we want to leave that return value to lua.
        return false;
    }

	lua_remove(State, -2); // Remove the traceback.
    return true;
}

bool LuaManager::is_valid() const {
    return State != nullptr;
}

bool LuaManager::register_function(const lua_CFunction function, const std::string &function_name) const {
    if (!function || function_name.empty())
        return false;
    lua_register(State, function_name.c_str(), function);
    return true;
}

int LuaManager::get_global_i(const std::string &variable_name, const int defaultv)
{
    const auto value = luabridge::getGlobal(State, variable_name.c_str());
    return to_int_or_default(value, defaultv);
}

std::string LuaManager::get_global_s(const std::string &variable_name, std::string defaultv)
{
    const auto value = luabridge::getGlobal(State, variable_name.c_str());
    return to_string_or_default(value, defaultv);
}

double LuaManager::get_global_d(const std::string &variable_name, const double Default)
{
    const auto value = luabridge::getGlobal(State, variable_name.c_str());
    return to_double_or_default(value, Default);
}

void LuaManager::set_global(const std::string &VariableName, const std::string &Value)
{
    luabridge::setGlobal(State, Value, VariableName.c_str());
}

void LuaManager::set_global(const std::string &VariableName, const int Value)
{
    luabridge::setGlobal(State, Value, VariableName.c_str());
}

void LuaManager::set_global(const std::string &VariableName, const double &Value)
{
    luabridge::setGlobal(State, Value, VariableName.c_str());
}

void LuaManager::set_global(const std::string &VariableName, const bool Value)
{
    luabridge::setGlobal(State, Value, VariableName.c_str());
}

bool LuaManager::register_struct(std::string Key, void* data, std::string MetatableName)
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

void* LuaManager::get_struct(std::string Key)
{
    void* ptr = nullptr;
    lua_pushstring(State, Key.c_str());
    lua_gettable(State, LUA_REGISTRYINDEX);
    ptr = lua_touserdata(State, -1); // returns null if does not exist

    pop();
    return ptr;
}

void LuaManager::new_array()
{
    lua_newtable(State);
}

bool LuaManager::use_array(std::string variable_name)
{
    get_global(variable_name);
    if (lua_istable(State, -1))
        return true;

    pop();
    return false;
}

void LuaManager::set_field_i(const int index, const int value)
{
    auto table = luabridge::LuaRef::fromStack(State, -1);
    table[index] = value;
}

void LuaManager::set_field_i(std::string name, const int value)
{
    auto table = luabridge::LuaRef::fromStack(State, -1);
    table[name] = value;
}

void LuaManager::set_field_s(const int index, std::string value)
{
    auto table = luabridge::LuaRef::fromStack(State, -1);
    table[index] = value;
}

void LuaManager::set_field_s(std::string name, std::string value)
{
    auto table = luabridge::LuaRef::fromStack(State, -1);
    table[name] = value;
}

void LuaManager::set_field_d(const int index, const double value)
{
    auto table = luabridge::LuaRef::fromStack(State, -1);
    table[index] = value;
}

void LuaManager::set_field_d(std::string name, const double value)
{
    auto table = luabridge::LuaRef::fromStack(State, -1);
    table[name] = value;
}

int LuaManager::get_field_i(std::string key, const int Default)
{
    const auto table = luabridge::LuaRef::fromStack(State, -1);
    const auto value = table[key];
    return to_int_or_default(value, Default);
}

double LuaManager::get_field_d(std::string key, const double Default)
{
    const auto table = luabridge::LuaRef::fromStack(State, -1);
    if (!table.isTable())
        return Default;

    const auto value = table[key];
    return to_double_or_default(value, Default);
}

std::string LuaManager::get_field_s(std::string key, std::string Default)
{
    const auto table = luabridge::LuaRef::fromStack(State, -1);
    const auto value = table[key];
    return to_string_or_default(value, Default);
}

void LuaManager::pop()
{
    lua_pop(State, 1);
}

void LuaManager::finalize_array(std::string ArrayName)
{
    lua_setglobal(State, ArrayName.c_str());
}

void LuaManager::finalize_enum(std::string EnumName)
{
	// create read-only metatable
	new_array();
	lua_pushcfunction(State, LuaReadOnlyError);
	lua_setfield(State, -2, "__newindex");
	lua_setmetatable(State, -2);
	
	lua_setglobal(State, EnumName.c_str());
}

void LuaManager::append_path(std::string Path)
{
    get_global("package");
    set_field_s("path", get_field_s("path") + ";" + Path);
    pop();
}

void LuaManager::push_argument(const int Value)
{
    if (func_input)
        lua_pushnumber(State, Value);
}

void LuaManager::push_argument(const double Value)
{
    if (func_input)
        lua_pushnumber(State, Value);
}

void LuaManager::push_argument(std::string Value)
{
    if (func_input)
        lua_pushstring(State, Value.c_str());
}

void LuaManager::push_argument(const bool Value)
{
	if (func_input)
		lua_pushboolean(State, Value);
}

int LuaManager::get_stack_top()
{
    return lua_gettop(State);
}

// http://lua-users.org/lists/lua-l/2006-03/msg00335.html
void LuaManager::dump_stack()
{
	auto L = State;
	int i = lua_gettop(L);
	/*Log::LogPrintf(" ----------------  Stack Dump ----------------\n");
	while (i) {
		int t = lua_type(L, i);
		switch (t) {
		case LUA_TSTRING:
			Log::LogPrintf("%d:`%s'\n", i, lua_tostring(L, i));
			break;
		case LUA_TBOOLEAN:
			Log::LogPrintf("%d: %s\n", i, lua_toboolean(L, i) ? "true" : "false");
			break;
		case LUA_TNUMBER:
			Log::LogPrintf("%d: %g\n", i, lua_tonumber(L, i));
			break;
		default: Log::LogPrintf("%d: %s\n", i, lua_typename(L, t)); break;
		}
		i--;
	}
	Log::LogPrintf("--------------- Stack Dump Finished ---------------\n");*/
}

bool LuaManager::call_function(const char* Name, const int Arguments, const int Results)
{
	bool IsFunc;

	func_args = Arguments;
	func_results = Results;

	bool isTable = lua_istable(State, -1);
	if (isTable) {
		lua_pushstring(State, Name);
		lua_gettable(State, -2);
	}
	else {
		lua_getglobal(State, Name);
	}
    IsFunc = lua_isfunction(State, -1);

    if (IsFunc)
        func_input = true;
	else {
		pop();

		// this is generalizable, but i'm too lazy. -az
		if (isTable) {
			lua_getglobal(State, Name);

			IsFunc = lua_isfunction(State, -1);
			if (IsFunc)
			{
				func_input = true;
			}
			else
				pop();
		}
	}

    return IsFunc;
}

bool LuaManager::run_function()
{
    if (!func_input)
        return false;
    func_input = false;

	int base = lua_gettop(State) - func_args;
	lua_pushcfunction(State, LuaPanic);
	lua_insert(State, base);

    int errc = lua_pcall(State, func_args, func_results, base);

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
		lua_remove(State, base); // remove traceback function
        pop(); // Remove the error from the stack.
        func_err = true;
        return false;
    }

    func_err = false;
	lua_remove(State, base);
	// Remove traceback function.
    return true;
}

int LuaManager::get_function_result(const int StackPos)
{
    return get_function_result_d(StackPos);
}

std::string LuaManager::get_function_result_s(const int StackPos)
{
	std::string Value;

	if (func_err) return Value;

	if (lua_isstring(State, -StackPos))
	{
		Value = lua_tostring(State, -StackPos);
	}

	pop();
	return Value;
}

float LuaManager::get_stack_f(const int StackPos)
{
	return get_function_result_d(StackPos);
}

double LuaManager::get_function_result_d(const int StackPos)
{
	double Value = -1;

	if (func_err) return 0;

	if (lua_isnumber(State, -StackPos))
	{
		Value = lua_tonumber(State, -StackPos);
	}

	pop();
	return Value;
}

void LuaManager::new_metatable(std::string MtName)
{
    luaL_newmetatable(State, MtName.c_str());
}

void LuaManager::register_library(std::string Libname, const luaL_Reg *Reg)
{
    lua_newtable(State);
    luaL_setfuncs(State, Reg, 0);
    lua_setglobal(State, Libname.c_str());
}

lua_State* LuaManager::get_lua_state()
{
    return State;
}

void LuaManager::start_iteration()
{
    lua_pushnil(State);
}

bool LuaManager::iterate_next()
{
    return lua_next(State, -2) != 0;
}

int LuaManager::next_int()
{
    return lua_tonumber(State, -1);
}

double LuaManager::next_double()
{
    return lua_tonumber(State, -1);
}

std::string LuaManager::next_g_string()
{
    return lua_tostring(State, -1);
}
