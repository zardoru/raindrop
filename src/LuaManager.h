#ifndef LuaManager_H
#define LuaManager_H

extern "C" {

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

}

#include "Directory.h"

int LuaPanic(lua_State* State);

class LuaManager
{

	lua_State* State;
	void GetGlobal(GString VarName);
	int func_args, func_results; bool func_input; bool func_err;

public:

	LuaManager();
	~LuaManager();
	
	bool IsValid(); // returns true if instance is valid, as in we were able to open a lua state.

	// All functions here will crash if the lua state is not valid.

	bool Register(lua_CFunction Function, GString FunctionName);
	bool RegisterStruct(GString Key, void* data, GString MetatableName = GString());
	void RegisterLibrary(GString arrayname, const luaL_Reg *lib);
	void* GetStruct(GString Key);
	bool RunScript(Directory file);
	bool RunScript(GString Filename);
	bool RunGString(GString sGString);

	// Global variables

	int GetGlobalI(GString VariableName, int Default = -1);
	double GetGlobalD(GString VariableName, double Default = -1);
	GString GetGlobalS(GString VariableName, GString Default = GString());

	void SetGlobal(GString VariableName, GString Value);
	void SetGlobal(GString VariableName, double Value);

	lua_State* GetState();

	// Function calling
	void PushArgument(int Value);
	void PushArgument(double Value);
	void PushArgument(GString Value);

	bool CallFunction(const char* Name, int Arguments = 0, int Results = 0);
	bool RunFunction();

	int GetFunctionResult(int StackPos = 1);
	float GetFunctionResultF(int StackPos = 1);

	void Pop();

	/* Metatables */
	void NewMetatable(GString MtName);

	// Arrays
	/* 
	 pretend you're working with a state machine and these alter the state
	 you call newarray or usearray if it exists, set fields 
	 then call FinalizeArray if you are done with it and your call 
	 for working with it was newarray() 
	 also these are tables but we work with them differently than how we would do tables.
    */

	void NewArray();
	bool UseArray(GString VariableName); // returns true if the array exists
	
	void SetFieldI(int index, int Value);
	void SetFieldD(int index, double Value);
	void SetFieldS(int index, GString Value);

	int GetFieldI(GString Key, int Default = -1);
	double GetFieldD(GString Key, double Default = -1);
	GString GetFieldS(GString Key, GString Default = GString());

	// Table iteration
	void StartIteration ();

	bool IterateNext();
	int NextInt();
	double NextDouble();
	GString NextGString();
	
	void FinalizeArray(GString ArrayName); // saves the new array with this name

	// TODO: Table variables

};

template <class T>
T* GetObjectFromState(lua_State* L, GString ObjectName)
{
	lua_pushstring(L, ObjectName.c_str());
    lua_gettable(L, LUA_REGISTRYINDEX);
    return (T*)lua_touserdata(L, -1);
}

template<class T>
T* GetUserObject(lua_State *L, int Parameter, const char* MetatableName)
{
	T* ud = (T*)luaL_checkudata(L, Parameter, MetatableName);
	luaL_argcheck(L, ud != NULL, 1, "Expected object of different type!");
	return ud;
}

#endif
