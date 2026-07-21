#include <filesystem>
#include <functional>

#include "../structure/SceneEnvironment.h"
#include "LuaManager.h"

#include <LuaBridge/LuaBridge.h>



/// @engineclass SceneEnvironment
void CreateSceneEnvironmentLua(LuaManager* anim_lua)
{
	/// Object2D handler. Does most callbacks behind the scene.
	// @type SceneEnvironment
	luabridge::getGlobalNamespace(anim_lua->get_lua_state())
		.beginClass <SceneEnvironment>("GraphObjMan")
		/// Queue a textured quad for drawing after Update.
		// @function DrawQuad
		.addFunction("DrawQuad", static_cast<void (SceneEnvironment::*)(uint32_t, Texture2D *, Transformation *, const ColorRGBA &, int)>(&SceneEnvironment::draw_quad))
		/// Queue text for drawing after Update.
		// @function DrawString
		.addFunction("DrawString", static_cast<void (SceneEnvironment::*)(uint32_t, Font *, std::string, const Vec2 &, float)>(&SceneEnvironment::draw_string))
		.addFunction("DrawString", static_cast<void (SceneEnvironment::*)(uint32_t, Font *, std::string, const Vec2 &, float, float)>(&SceneEnvironment::draw_string))
		.addFunction("DrawString", static_cast<void (SceneEnvironment::*)(uint32_t, Font *, std::string, const Vec2 &, float, const ColorRGBA &)>(&SceneEnvironment::draw_string))
		.addFunction("DrawString", static_cast<void (SceneEnvironment::*)(uint32_t, Font *, std::string, const Vec2 &, float, const ColorRGBA &, float)>(&SceneEnvironment::draw_string))
		/// Register a previously-unregistered @{Object2D}
		// @function AddTarget
		// @tparam Object2Dtarget The Object2D to handle.
		.addFunction("AddTarget", &SceneEnvironment::add_sprite_target)
        /// Register a previously-unregistered @{Object2D}
        // @function AddTarget
        // @tparam Object2Dtarget The Object2D to handle.
        .addFunction("RemoveTarget", &SceneEnvironment::remove_sprite_target)
		/// Sort objects. Generally done behind-the-scenes when a Z is changed.
		// @function Sort
		.addFunction("Sort", &SceneEnvironment::sort)
		/// Create an Object2D and register it. Shorthand for obj = Object2D(); Engine:AddTarget(obj).
		// @function CreateObject
		// @return A new @{Object2D}
		.addFunction("CreateObject", &SceneEnvironment::create_object)
		.endClass();

	luabridge::push(anim_lua->get_lua_state(), get_object_from_state<SceneEnvironment>(anim_lua->get_lua_state(), "GOMAN"));
	lua_setglobal(anim_lua->get_lua_state(), "Engine");
}
