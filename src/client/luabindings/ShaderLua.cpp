#include <string>
#include <glm.h>
#include <filesystem>

#include "Shader.h"
#include "Rendering.h"
#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

class LShader : public renderer::Shader {
    struct Locations {
        int projection = -1;
        int model_view = -1;
        int centered = -1;
        int color = -1;
    } locations_;

public:
	void Compile(const std::string& fragment) {
		renderer::Shader::compile(fragment);
		if (is_valid()) {
			locations_.projection = get_uniform("projection");
			locations_.model_view = get_uniform("mvp");
			locations_.centered = get_uniform("centered");
			locations_.color = get_uniform("color");
		}
	}

    void apply_draw_state(const DrawState &state) const override {
        if (locations_.projection != -1)
            set_uniform(locations_.projection, &state.projection[0][0]);
        if (locations_.model_view != -1 && state.model)
            set_uniform(locations_.model_view, &(*state.model)[0][0]);
        if (locations_.centered != -1)
            set_uniform(locations_.centered, state.centered);
        if (locations_.color != -1)
            set_uniform(locations_.color,
                        l2gamma(state.color.red), l2gamma(state.color.green), l2gamma(state.color.blue), state.color.alpha);
    }

	int Send(lua_State *L) {
		int n = lua_gettop(L);
		std::string sendto = luaL_checkstring(L, 2);

		bind();
		int uniform = Shader::get_uniform(sendto);

		switch (n) {
		case 3:
			Shader::set_uniform(uniform, (float)luaL_checknumber(L, 3));
			break;
		case 4:
			Shader::set_uniform(uniform, Vec2((float)luaL_checknumber(L, 3),
				(float)luaL_checknumber(L, 4)));
			break;
		case 5:
			Shader::set_uniform(uniform, Vec3((float)luaL_checknumber(L, 3),
				(float)luaL_checknumber(L, 4),
				(float)luaL_checknumber(L, 5)));
			break;
		case 6:
			Shader::set_uniform(uniform, (float)luaL_checknumber(L, 3),
				(float)luaL_checknumber(L, 4),
				(float)luaL_checknumber(L, 5),
				(float)luaL_checknumber(L, 6));
			break;
		default:
			return luaL_error(L, "shader send has wrong argument n (%d) range is 3 to 6", n + 1);
		};

		return 0;
	}
};

/// @engineclass Shader
void CreateShaderLua(LuaManager* anim_lua)
{
	luabridge::getGlobalNamespace(anim_lua->get_lua_state())
		.beginClass<renderer::Shader>("__shader_internal")
		.endClass()
		.deriveClass<LShader, renderer::Shader>("Shader")
		/// Creates a new shader instance.
		// @function Shader
		.addConstructor<void(*) ()>()
		/// Compiles a fragment shader.
		// @function Compile
		// @param frag The fragment shader contents.
		.addFunction("Compile", &LShader::Compile)
		/// Sends a variable to a shader.
		// @function Send
		// @param name The variable name of the shader.
		// @param val1 First value.
		// @param[opt] val2 Second value.
		// @param[optchain] val3 Third value.
		// @param[optchain] val4 Fourth value.
		.addCFunction("Send", &LShader::Send)
		.endClass();
}
