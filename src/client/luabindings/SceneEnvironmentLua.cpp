#include <filesystem>
#include <functional>

#include "../structure/SceneEnvironment.h"
#include "LuaManager.h"

#include <LuaBridge/LuaBridge.h>



/// @engineclass SceneEnvironment
void CreateSceneEnvironmentLua(LuaManager* anim_lua)
{
	/// Easing constants
	// @enum Easing
	// @param None No easing
	// @param In Ease In
	// @param Out Ease Out
	anim_lua->NewArray();
	anim_lua->SetFieldI("None", Animation::EaseLinear);
	anim_lua->SetFieldI("In", Animation::EaseIn);
	anim_lua->SetFieldI("Out", Animation::EaseOut);
	anim_lua->FinalizeEnum("Easing");

	/// Object2D handler. Does most callbacks behind the scene.
	// @type SceneEnvironment
	luabridge::getGlobalNamespace(anim_lua->GetState())
		.beginClass <SceneEnvironment>("GraphObjMan")
		/// Add an animation to be handled by SceneEnvironment.
		// @function AddAnimation
		// @tparam Object2D target Target Object2D
		// @tparam string funcname Name of the Lua function on the global scope
		// @tparam Easing easing Easing type.
		// @tparam double duration Duration in seconds.
		// @tparam double delay Time in seconds to wait for the animation to fire.
		.addFunction("AddAnimation", &SceneEnvironment::AddLuaAnimation)
		/// Register a previously-unregistered @{Object2D}
		// @function AddTarget
		// @tparam Object2Dtarget The Object2D to handle.
		.addFunction("AddTarget", &SceneEnvironment::AddSpriteTarget)
		/// Sort objects. Generally done behind-the-scenes when a Z is changed.
		// @function Sort
		.addFunction("Sort", &SceneEnvironment::Sort)
		/// Stop all animations of the target.
		// @function StopAnimation
		// @param target Target to stop animations of.
		.addFunction("StopAnimation", &SceneEnvironment::StopAnimationsForTarget)
		/// Create an Object2D and register it. Shorthand for obj = Object2D(); Engine:AddTarget(obj).
		// @function CreateObject
		// @return A new @{Object2D}
		.addFunction("CreateObject", &SceneEnvironment::CreateObject)
		.endClass();

	luabridge::push(anim_lua->GetState(), GetObjectFromState<SceneEnvironment>(anim_lua->GetState(), "GOMAN"));
	lua_setglobal(anim_lua->GetState(), "Engine");
}