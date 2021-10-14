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
        obj->ChainTransformation(param);
    }
    static VectorLua getScaleVec(Transformation const *obj) {
        return {obj->GetScaleX(), obj->GetScaleY()};
    }

    static void setScaleVec(Transformation *obj, VectorLua v) {
        obj->SetScaleX(v.getX());
        obj->SetScaleY(v.getY());
    }

    static VectorLua getSize(Transformation const *obj) {
        return {obj->GetWidth(), obj->GetHeight()};
    }

    static void setSize(Transformation *obj, VectorLua v) {
        obj->SetWidth(v.getX());
        obj->SetHeight(v.getY());
    }

    static VectorLua getPosition(Transformation const *obj) {
        return {obj->GetPositionX(), obj->GetPositionX()};
    }

    static void setPosition(Transformation *obj, VectorLua v) {
        obj->SetPositionX(v.getX());
        obj->SetPositionY(v.getY());
    }

    static AABB getRect(Transformation const *obj) {
        return {
                obj->GetPositionX(),
                obj->GetPositionY(),
                obj->GetPositionX() + obj->GetWidth(),
                obj->GetPositionY() + obj->GetHeight()
        };
    }

    static void setRect(Transformation *obj, AABB box) {
        obj->SetPositionX(box.X1);
        obj->SetPositionY(box.X2);
        obj->SetWidth(box.width());
        obj->SetHeight(box.height());
    }
    
};

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