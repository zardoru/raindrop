#include <lua.hpp>

int LuaPanic(lua_State* State);

class LuaManager
{
	bool WeOwnThisState;
    lua_State* State;
    void get_global(std::string VarName);

    int func_args, func_results; bool func_input; bool func_err;
	std::string last_error;

public:

    LuaManager();
	LuaManager(lua_State* state);

    ~LuaManager();

    bool is_valid() const; // returns true if instance is valid, as in we were able to open a lua state.

    // All functions here will crash if the lua state is not valid.

    bool Register(lua_CFunction function, const std::string &function_name) const;
    bool register_struct(std::string Key, void* data, std::string MetatableName = std::string());
    void register_library(std::string arrayname, const luaL_Reg *lib);
    void* get_struct(std::string Key);
    bool run_script(std::filesystem::path file);
    bool run_script(std::string Filename);
    
    bool run_string(std::string string);

    // Do a "require" call with Filename as the argument. This leaves a value on the stack!
    bool require(std::filesystem::path path);

	void dump_stack();

    // Global variables

    int get_global_i(const std::string &variable_name, int defaultv = -1);
    double get_global_d(const std::string &variable_name, double Default = -1);
    std::string get_global_s(const std::string &variable_name, std::string defaultv = std::string());

    void set_global(const std::string &VariableName, const std::string &Value);
    void set_global(const std::string &VariableName, int Value);
    void set_global(const std::string &VariableName, const double &Value);
    void set_global(const std::string &VariableName, bool Value);

    lua_State* get_lua_state();

    // Function calling
    void push_argument(int Value);
    void push_argument(double Value);
    void push_argument(std::string Value);
	void push_argument(bool Value);

    bool call_function(const char* Name, int Arguments = 0, int Results = 0);
    bool run_function();

    int get_function_result(int StackPos = 1);
	std::string get_function_result_s(int StackPos = 1);
    float get_stack_f(int StackPos = 1);
	double get_function_result_d(int StackPos = 1);

    std::string get_last_error();

    void pop();

    /* Metatables */
    void new_metatable(std::string MtName);

    int get_stack_top();

    // Arrays
    /*
     pretend you're working with a state machine and these alter the state
     you call newarray or usearray if it exists, set fields
     then call FinalizeArray if you are done with it and your call
     for working with it was newarray()
     also these are tables but we work with them differently than how we would do tables.
    */

    void new_array();
    bool use_array(std::string variable_name); // returns true if the array exists

    void set_field_i(int index, int value);
	void SetFieldI(std::string name, int value);
    void set_field_d(int index, double value);
	void set_field_d(std::string name, double value);
    void set_field_s(int index, std::string value);
    void set_field_s(std::string name, std::string value);

    int get_field_i(std::string key, int Default = -1);
    double get_field_d(std::string key, double Default = -1);
    std::string get_field_s(std::string key, std::string Default = std::string());

    // Table iteration
    void start_iteration();

    bool iterate_next();
    int next_int();
    double next_double();
    std::string next_g_string();

    void finalize_array(std::string ArrayName); // saves the new array with this name
	void finalize_enum(std::string EnumName);
    void append_path(std::string Path);
    // TODO: Table variables
};

template <class T>
T* GetObjectFromState(lua_State* L, const std::string ObjectName)
{
    lua_pushstring(L, ObjectName.c_str());
    lua_gettable(L, LUA_REGISTRYINDEX);
    return (T*)lua_touserdata(L, -1);
}

template<class T>
T* GetUserObject(lua_State *L, const int Parameter, const char* MetatableName)
{
    T* ud = (T*)luaL_checkudata(L, Parameter, MetatableName);
    luaL_argcheck(L, ud != NULL, 1, "Expected object of different type!");
    return ud;
}

void AddRDLuaGlobal(LuaManager * anim_lua);
