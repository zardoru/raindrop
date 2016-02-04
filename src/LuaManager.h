#pragma once

int LuaPanic(lua_State* State);

class LuaManager
{
    lua_State* State;
    void GetGlobal(std::string VarName);

    int func_args, func_results; bool func_input; bool func_err;

public:

    LuaManager();
    ~LuaManager();

    bool IsValid(); // returns true if instance is valid, as in we were able to open a lua state.

    // All functions here will crash if the lua state is not valid.

    bool Register(lua_CFunction Function, std::string FunctionName);
    bool RegisterStruct(std::string Key, void* data, std::string MetatableName = std::string());
    void RegisterLibrary(std::string arrayname, const luaL_Reg *lib);
    void* GetStruct(std::string Key);
    bool RunScript(Directory file);
    bool RunScript(std::string Filename);
    bool RunString(std::string sGString);

    // Do a "require" call with Filename as the argument. This leaves a value on the stack!
    bool Require(std::string Filename);

    // Global variables

    int GetGlobalI(std::string VariableName, int Default = -1);
    double GetGlobalD(std::string VariableName, double Default = -1);
    std::string GetGlobalS(std::string VariableName, std::string Default = std::string());

    void SetGlobal(const std::string &VariableName, const std::string &Value);
    void SetGlobal(const std::string &VariableName, const double &Value);

    lua_State* GetState();

    // Function calling
    void PushArgument(int Value);
    void PushArgument(double Value);
    void PushArgument(std::string Value);

    bool CallFunction(const char* Name, int Arguments = 0, int Results = 0);
    bool RunFunction();

    int GetFunctionResult(int StackPos = 1);
    float GetFunctionResultF(int StackPos = 1);

    void Pop();

    /* Metatables */
    void NewMetatable(std::string MtName);

    // Arrays
    /*
     pretend you're working with a state machine and these alter the state
     you call newarray or usearray if it exists, set fields
     then call FinalizeArray if you are done with it and your call
     for working with it was newarray()
     also these are tables but we work with them differently than how we would do tables.
    */

    void NewArray();
    bool UseArray(std::string VariableName); // returns true if the array exists

    void SetFieldI(int index, int Value);
    void SetFieldD(int index, double Value);
    void SetFieldS(int index, std::string Value);
    void SetFieldS(std::string name, std::string Value);

    int GetFieldI(std::string Key, int Default = -1);
    double GetFieldD(std::string Key, double Default = -1);
    std::string GetFieldS(std::string Key, std::string Default = std::string());

    // Table iteration
    void StartIteration();

    bool IterateNext();
    int NextInt();
    double NextDouble();
    std::string NextGString();

    void FinalizeArray(std::string ArrayName); // saves the new array with this name
    void AppendPath(std::string Path);
    // TODO: Table variables
};

template <class T>
T* GetObjectFromState(lua_State* L, std::string ObjectName)
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
