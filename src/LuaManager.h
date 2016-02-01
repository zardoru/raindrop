#pragma once

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
	bool RunString(GString sGString);

	// Do a "require" call with Filename as the argument. This leaves a value on the stack!
	bool Require(GString Filename);

	// Global variables

	int GetGlobalI(GString VariableName, int Default = -1);
	double GetGlobalD(GString VariableName, double Default = -1);
	GString GetGlobalS(GString VariableName, GString Default = GString());

	void SetGlobal(const GString &VariableName, const GString &Value);
	void SetGlobal(const GString &VariableName, const double &Value);

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
	void SetFieldS(GString name, GString Value);

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
	void AppendPath(GString Path);
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
