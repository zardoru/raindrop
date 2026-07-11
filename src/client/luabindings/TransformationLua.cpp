#include <filesystem>

#include "Transformation.h"
#include "LuaManager.h"
#include "VectorLua.h"
#include <LuaBridge/LuaBridge.h>
#include <rmath.h>

class TransformationProxy {
public:
    template<class T>
    static Transformation getChainTransformation(T const *obj) {
        return Transformation();
    }

    template<class T>
    static void setChainTransformation(T *obj, Transformation *param) {
        obj->chain_transformation(param);
    }
    static VectorLua getScaleVec(Transformation const *obj) {
        return {obj->get_scale_x(), obj->get_scale_y()};
    }

    static void setScaleVec(Transformation *obj, VectorLua v) {
        obj->set_scale_x(v.getX());
        obj->set_scale_y(v.getY());
    }

    static VectorLua getSize(Transformation const *obj) {
        return {obj->get_width(), obj->get_height()};
    }

    static void setSize(Transformation *obj, VectorLua v) {
        obj->set_width(v.getX());
        obj->set_height(v.getY());
    }

    static VectorLua getPosition(Transformation const *obj) {
        return {obj->get_position_x(), obj->get_position_y()};
    }

    static void setPosition(Transformation *obj, VectorLua v) {
        obj->set_position_x(v.getX());
        obj->set_position_y(v.getY());
    }

    static AABB getRect(Transformation const *obj) {
        return {
                obj->get_position_x(),
                obj->get_position_y(),
                obj->get_position_x() + obj->get_width(),
                obj->get_position_y() + obj->get_height()
        };
    }

    static void setRect(Transformation *obj, AABB box) {
        obj->set_position_x(box.X1);
        obj->set_position_y(box.X2);
        obj->set_width(box.width());
        obj->set_height(box.height());
    }
    
};

/// Transformation class to have an object hierarchy.
/// @engineclass Transformation
void CreateTransformationLua(LuaManager* anim_lua)
{
	
	luabridge::getGlobalNamespace(anim_lua->get_lua_state())
		.beginClass <Transformation>("Transformation")
		/// Transformation constructor
		// @function Transformation
		.addConstructor<void(*) ()>()
		/// Transformation class to have an object hierarchy.
		// @type Transformation


		/// Layer shorthand.
		// @property Z
		.addProperty("Z", &Transformation::get_z, &Transformation::set_z)
		/// Layer. Does the same as the layer of @{Object2D}
		// @property Layer
		.addProperty("Layer", &Transformation::get_z, &Transformation::set_z)
		/// Rotation in degrees.
		// @property Rotation
		.addProperty("Rotation", &Transformation::get_rotation, &Transformation::set_rotation)
		/// Transformation Width. Stacks with ScaleX
		// @property Width
		.addProperty("Width", &Transformation::get_width, &Transformation::set_width)
		/// Transformation height. Stacks with ScaleY.
		// @property Height
		.addProperty("Height", &Transformation::get_height, &Transformation::set_height)
		/// Horizontal Scale in local space.
		// @property ScaleX
		.addProperty("ScaleX", &Transformation::get_scale_x, &Transformation::set_scale_x)
		/// Vertical scale in local space.
		// @property ScaleY
		.addProperty("ScaleY", &Transformation::get_scale_y, &Transformation::set_scale_y)
		/// X position in local space.
		// @property X
		.addProperty("X", &Transformation::get_position_x, &Transformation::set_position_x)
		/// Y position in local space.
		// @property Y
		.addProperty("Y", &Transformation::get_position_y, &Transformation::set_position_y)
       .addProperty("ChainTransformation",
                    &TransformationProxy::getChainTransformation<Transformation>,
                    &TransformationProxy::setChainTransformation<Transformation>)
       .addProperty("Parent",
                    &TransformationProxy::getChainTransformation<Transformation>,
                    &TransformationProxy::setChainTransformation<Transformation>)
               // TODO: Document Lua
       .addProperty("Position", &TransformationProxy::getPosition, &TransformationProxy::setPosition)
       .addProperty("position", &TransformationProxy::getPosition, &TransformationProxy::setPosition)
       .addProperty("ScaleVec", &TransformationProxy::getScaleVec, &TransformationProxy::setScaleVec)
       .addProperty("scaleVec", &TransformationProxy::getScaleVec, &TransformationProxy::setScaleVec)
       .addProperty("Size", &TransformationProxy::getSize, &TransformationProxy::setSize)
       .addProperty("size", &TransformationProxy::getSize, &TransformationProxy::setSize)
       .addProperty("Rect", &TransformationProxy::getRect, &TransformationProxy::setRect)
       .addProperty("rect", &TransformationProxy::getRect, &TransformationProxy::setRect)
//		/// Set to have a transformation to apply after this one.
//		// @function SetChainTransformation
//		// @tparam Transformation A transformation to chain to this one.
//		.addFunction("SetChainTransformation", &Transformation::ChainTransformation)
		.endClass();
}
