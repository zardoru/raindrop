#include <filesystem>
#include <rmath.h>

#include <glm.h>

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include "VectorLua.h"

VectorLua::VectorLua(float x, float y) {
    vec2 = glm::vec2(x, y);
}

VectorLua VectorLua::add(VectorLua v) {
    return VectorLua(vec2 + v.vec2);
}

VectorLua VectorLua::sub(VectorLua v) {
    return VectorLua(vec2 - v.vec2);
}

float VectorLua::dot(VectorLua v) {
    return glm::dot(vec2, v.vec2);
}

void VectorLua::setX(float x) {
    vec2.x = x;
}

void VectorLua::setY(float y) {
    vec2.y = y;
}

float VectorLua::getX() const {
    return vec2.x;
}

float VectorLua::getY() const {
    return vec2.y;
}

VectorLua::VectorLua(glm::vec2 vec) {
    vec2 = vec;
}

VectorLua VectorLua::scale(float v) {
    return VectorLua(vec2 * v);
}

class AABBproxy {
public:
    static VectorLua getPosition(AABB const *self) {
        return {self->X1, self->Y1};
    }

    static void setPosition(AABB *self, VectorLua v) {
        self->X1 = v.getX();
        self->Y1 = v.getY();
    }

    static VectorLua getSize(AABB const *self) {
        return {self->width(), self->height()};
    }

    static void setSize(AABB *self, VectorLua v) {
        self->SetWidth(v.getX());
        self->SetHeight(v.getY());
    }
};

/// Utility classes.
/// @engineclass Utility
void CreateUtilityLua(LuaManager *AnimLua) {
    luabridge::getGlobalNamespace(AnimLua->GetState())
            .beginClass<VectorLua>("Vec2")
            // TODO: Document Lua
            .addConstructor < void(*)(float, float) > ()
            .addProperty("X", &VectorLua::getX, &VectorLua::setX)
            .addProperty("x", &VectorLua::getX, &VectorLua::setX)
            .addProperty("Y", &VectorLua::getY, &VectorLua::setY)
            .addProperty("y", &VectorLua::getY, &VectorLua::setY)
            .addFunction("add", &VectorLua::add)
            .addFunction("sub", &VectorLua::sub)
            .addFunction("dot", &VectorLua::dot)
            .addFunction("scale", &VectorLua::scale)
            .endClass()
            .beginClass<AABB>("AABB")
                    /// Constructor
                    // @function AABB
            .addConstructor < void(*)(void) > () // non-functional
            /// Second version of constructor
            // @function AABB
            // @param x1 Left of AABB
            // @param y1 Top of AABB
            // @param x2 Right of AABB
            // @param y2 Bottom of AABB
            .addConstructor < void(*)(float, float, float, float) > ()

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

            .addProperty("position", &AABBproxy::getPosition, &AABBproxy::setPosition)
            .addProperty("size", &AABBproxy::getSize, &AABBproxy::setSize)
                    /// Whether a point is within the AABB
                    // @function Contains
                    // @param x X coordinate
                    // @param y Y coordinate
            .addFunction("Contains", &AABB::IsInBox)
            .addFunction("contains", &AABB::IsInBox)
                    /// Whether this AABB intersects another AABB
                    // @function Intersects
                    // @tparam AABB aabb Other AABB
            .addFunction("Intersects", &AABB::Intersects)
            .addFunction("intersects", &AABB::Intersects)
            .endClass();
}