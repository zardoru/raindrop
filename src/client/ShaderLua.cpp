#include <string>
#include <glm.h>
#include <filesystem>

#include "Shader.h"
#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

class LShader : public Renderer::Shader {
public:
	int Send(lua_State *L) {
		int n = lua_gettop(L);
		std::string sendto = luaL_checkstring(L, 2);

		Bind();
		int uniform = Shader::GetUniform(sendto);

		switch (n) {
		case 3:
			Shader::SetUniform(uniform, (float)luaL_checknumber(L, 3));
			break;
		case 4:
			Shader::SetUniform(uniform, Vec2((float)luaL_checknumber(L, 3),
				(float)luaL_checknumber(L, 4)));
			break;
		case 5:
			Shader::SetUniform(uniform, Vec3((float)luaL_checknumber(L, 3),
				(float)luaL_checknumber(L, 4),
				(float)luaL_checknumber(L, 5)));
			break;
		case 6:
			Shader::SetUniform(uniform, (float)luaL_checknumber(L, 3),
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
	luabridge::getGlobalNamespace(anim_lua->GetState())
		.beginClass<Renderer::Shader>("__shader_internal")
		.endClass()
		.deriveClass<LShader, Renderer::Shader>("Shader")
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