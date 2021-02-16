#include <filesystem>

#include "Transformation.h"
#include "../LuaManager.h"
#include <LuaBridge/LuaBridge.h>

/// Transformation class to have an object hierarchy.
/// @engineclass Transformation
void CreateTransformationLua(LuaManager* anim_lua)
{
	
	luabridge::getGlobalNamespace(anim_lua->GetState())
		.beginClass <Transformation>("Transformation")
		/// Transformation constructor
		// @function Transformation
		.addConstructor<void(*) ()>()
		/// Transformation class to have an object hierarchy.
		// @type Transformation


		/// Layer shorthand.
		// @property Z
		.addProperty("Z", &Transformation::GetZ, &Transformation::SetZ)
		/// Layer. Does the same as the layer of @{Object2D}
		// @property Layer
		.addProperty("Layer", &Transformation::GetZ, &Transformation::SetZ)
		/// Rotation in degrees.
		// @property Rotation
		.addProperty("Rotation", &Transformation::GetRotation, &Transformation::SetRotation)
		/// Transformation Width. Stacks with ScaleX
		// @property Width
		.addProperty("Width", &Transformation::GetWidth, &Transformation::SetWidth)
		/// Transformation height. Stacks with ScaleY.
		// @property Height
		.addProperty("Height", &Transformation::GetHeight, &Transformation::SetHeight)
		/// Horizontal Scale in local space.
		// @property ScaleX
		.addProperty("ScaleX", &Transformation::GetScaleX, &Transformation::SetScaleX)
		/// Vertical scale in local space.
		// @property ScaleY
		.addProperty("ScaleY", &Transformation::GetScaleY, &Transformation::SetScaleY)
		/// X position in local space.
		// @property X
		.addProperty("X", &Transformation::GetPositionX, &Transformation::SetPositionX)
		/// Y position in local space.
		// @property Y
		.addProperty("Y", &Transformation::GetPositionY, &Transformation::SetPositionY)
		/// Set to have a transformation to apply after this one.
		// @function SetChainTransformation
		// @tparam Transformation A transformation to chain to this one.
		.addFunction("SetChainTransformation", &Transformation::ChainTransformation)
		.endClass();
}