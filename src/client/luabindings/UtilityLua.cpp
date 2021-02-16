#include <filesystem>
#include <rmath.h>

#include "../LuaManager.h"
#include <LuaBridge/LuaBridge.h>

/// Utility classes.
/// @engineclass Utility
void CreateUtilityLua(LuaManager *AnimLua)
{
	luabridge::getGlobalNamespace(AnimLua->GetState())
		.beginClass<AABB>("AABB")
		/// Constructor
		// @function AABB
		.addConstructor<void(*)()>()
		/// Second version of constructor
		// @function AABB
		// @param x1 Left of AABB
		// @param y1 Top of AABB
		// @param x2 Right of AABB
		// @param y2 Bottom of AABB
		.addConstructor<void(*)(float, float, float, float)>()

		/// @type AABB
		
		/// Left of the AABB
		// @property x
		.addData("x", &AABB::X1)
		/// Right of the AABB
		// @property x2
		.addData("x2", &AABB::X2)

		/// Top of the AABB
		// @property y
		.addData("y", &AABB::Y1)

		/// Bottom of the AABB
		// @property y2
		.addData("y2", &AABB::Y2)
		/// Width of the AABB.
		// @property w
		.addProperty("w", &AABB::width, &AABB::SetWidth)
		/// Height of the AABB
		// @property h
		.addProperty("h", &AABB::height, &AABB::SetHeight)
		/// Whether a point is within the AABB
		// @function Contains
		// @param x X coordinate
		// @param y Y coordinate
		.addFunction("Contains", &AABB::IsInBox)
		/// Whether this AABB intersects another AABB
		// @function Intersects
		// @tparam AABB aabb Other AABB
		.addFunction("Intersects", &AABB::Intersects)
		.endClass();
}