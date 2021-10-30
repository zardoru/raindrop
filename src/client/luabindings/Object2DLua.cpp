#include <string>
#include <rmath.h>
#include <filesystem>
#include <map>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"


#include "LuaManager.h"
#include "Logging.h"

#include <game/GameConstants.h>
#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"
#include <LuaBridge/LuaBridge.h>
#include "VectorLua.h"

/// @engineclass Object2D


// We need these to be able to work with gcc.
// Adding these directly does not work. Inheriting them from Transformation does not work. We're left only with this.
struct O2DProxy {
    static uint32_t getZ(Sprite const *obj) {
        return obj->GetZ();
    }

    static float getScaleX(Sprite const *obj) {
        return obj->GetScaleX();
    }

    static float getScaleY(Sprite const *obj) {
        return obj->GetScaleY();
    }

    static float getWidth(Sprite const *obj) {
        return obj->GetWidth();
    }

    static float getHeight(Sprite const *obj) {
        return obj->GetHeight();
    }

    static float getX(Sprite const *obj) {
        return obj->GetPositionX();
    }

    static float getY(Sprite const *obj) {
        return obj->GetPositionY();
    }

    static float getRotation(Sprite const *obj) {
        return obj->GetRotation();
    }

    static float getRed(Sprite const *obj) {
        return obj->Color.Red;
    }

    static void setGreen(Sprite *obj, float param) {
        obj->Color.Green = param;
    }

    static float getGreen(Sprite const *obj) {
        return obj->Color.Green;
    }

    static void setBlue(Sprite *obj, float param) {
        obj->Color.Blue = param;
    }

    static float getBlue(Sprite const *obj) {
        return obj->Color.Blue;
    }

    static void setRed(Sprite *obj, float param) {
        obj->Color.Red = param;
    }

    template<class T>
    static Transformation getChainTransformation(T const *obj) {
        return Transformation();
    }

    static void setZ(Sprite *obj, uint32_t nZ) {
        obj->SetZ(nZ);
    }

    static void setHeight(Sprite *obj, float param) {
        obj->SetHeight(param);
    }

    static void setWidth(Sprite *obj, float param) {
        obj->SetWidth(param);
    }

    static void setScaleY(Sprite *obj, float param) {
        obj->SetScaleY(param);
    }

    static void setScaleX(Sprite *obj, float param) {
        obj->SetScaleX(param);
    }

    static void setRotation(Sprite *obj, float param) {
        obj->SetRotation(param);
    }

    static void setX(Sprite *obj, float param) {
        obj->SetPositionX(param);
    }

    static void setY(Sprite *obj, float param) {
        obj->SetPositionY(param);
    }

    template<class T>
    static void setChainTransformation(T *obj, Transformation *param) {
        obj->ChainTransformation(param);
    }

    static void setScale(Sprite *obj, float param) {
        obj->SetScale(param);
    }

    static void AddRotation(Sprite *obj, float param) {
        obj->AddRotation(param);
    }

    static float getScale(Sprite const *obj) {
        return (obj->GetScaleX() + obj->GetScaleY()) / 2;
    }

    static VectorLua getScaleVec(Sprite const *obj) {
        return {obj->GetScaleX(), obj->GetScaleY()};
    }

    static void setScaleVec(Sprite *obj, VectorLua v) {
        obj->SetScaleX(v.getX());
        obj->SetScaleY(v.getY());
    }

    static VectorLua getSize(Sprite const *obj) {
        return {obj->GetWidth(), obj->GetHeight()};
    }

    static void setSize(Sprite *obj, VectorLua v) {
        obj->SetWidth(v.getX());
        obj->SetHeight(v.getY());
    }

    static VectorLua getPosition(Sprite const *obj) {
        return {obj->GetPositionX(), obj->GetPositionY()};
    }

    static void setPosition(Sprite *obj, VectorLua v) {
        obj->SetPositionX(v.getX());
        obj->SetPositionY(v.getY());
    }

    static AABB getRect(Sprite const *obj) {
        return {
                obj->GetPositionX(),
                obj->GetPositionY(),
                obj->GetPositionX() + obj->GetWidth(),
                obj->GetPositionY() + obj->GetHeight()
        };
    }

    static void setRect(Sprite *obj, AABB box) {
        obj->SetPositionX(box.X1);
        obj->SetPositionY(box.X2);
        obj->SetWidth(box.width());
        obj->SetHeight(box.height());
    }
};

// Wrapper functions
void SetImage(Sprite *O, std::string dir) {
    O->SetImage(GameState::GetInstance().GetSkinImage(dir));
    if (O->GetImage() == nullptr)
        Log::Printf("File %s could not be loaded.\n", dir.c_str());
}

std::string GetImage(const Sprite *O) {
    return O->GetImageFilename();
}

void CreateObject2DLua(LuaManager *anim_lua) {
    /// Blend modes allowed by Object2D.
    // @enum BlendMode
    // @param Add Addition blend mode.
    // @param Alpha Alpha blend mode. Default.
    anim_lua->NewArray();
    anim_lua->SetFieldI("Add", (int) BLEND_ADD);
    anim_lua->SetFieldI("Alpha", (int) BLEND_ALPHA);
    anim_lua->FinalizeEnum("BlendMode");

    ///
    luabridge::getGlobalNamespace(anim_lua->GetState())
            .deriveClass<Sprite, Transformation>("Object2D")
                    /// Creates a new Object2D instance. On a Noteskin context, will only be drawn with Render().
                    // Otherwise, will only be drawn if created with @{SceneEnvironment:CreateObject} or
                    // subscribed with @{SceneEnvironment:AddTarget}.
                    // @function Object2D
            .addConstructor < void(*)() > ()
            /// Main sprite interface.
            // @type Object2D

            /// Whether this object uses the center or the top left as the pivot. If true, centered.
            // @property Centered
            .addData("Centered", &Sprite::Centered)
            .addData("Lighten", &Sprite::Lighten)
            .addData("LightenFactor", &Sprite::LightenFactor)
            .addData("Scissor", &Sprite::Scissor)
            .addData("ScissorRegion", &Sprite::ScissorRegion)
                    /// Whether to invert the colors of this sprite. Useless if the shader is set.
                    // @property ColorInvert
            .addData("ColorInvert", &Sprite::ColorInvert)
                    /// The sprite's alpha.
                    // @property Alpha
            .addData("Alpha", &Sprite::Alpha)
                    /// The red value of the sprite. Range from 0 to 1. Will be multiplied
                    // @property Red
            .addProperty("Red", &O2DProxy::getRed, &O2DProxy::setRed)
                    /// The green value of the sprite. Range from 0 to 1. Will be multiplied
                    // @property Green
            .addProperty("Green", &O2DProxy::getGreen, &O2DProxy::setGreen)
                    /// The blue value of the sprite. Range from 0 to 1. Will be multiplied
                    // @property Blue
            .addProperty("Blue", &O2DProxy::getBlue, &O2DProxy::setBlue)
                    /// The blend mode for this sprite.
                    // @property BlendMode
                    // @see BlendMode
            .addProperty("BlendMode", &Sprite::GetBlendMode, &Sprite::SetBlendMode)
                    /// Set the crop of the sprite by pixel measurements.
                    // @function SetCropByPixels
                    // @param x1 Left X coordinate.
                    // @param x2 Right X coordinate.
                    // @param y1 Top Y coordinate.
                    // @param y2 Bottom Y coordinate.
            .addFunction("SetCropByPixels", &Sprite::SetCropByPixels)
                    /// Reset the crop to the whole image.
                    // @function ResetCrop
            .addFunction("ResetCrop", &Sprite::SetCropToWholeImage)
                    /// @{Shader} to render this sprite with.
                    // @property Shader
            .addProperty("Shader", &Sprite::GetShader, &Sprite::SetShader)
            // TODO: Document Lua
            .addProperty("Position", &O2DProxy::getPosition, &O2DProxy::setPosition)
            .addProperty("position", &O2DProxy::getPosition, &O2DProxy::setPosition)
            .addProperty("ScaleVec", &O2DProxy::getScaleVec, &O2DProxy::setScaleVec)
            .addProperty("scaleVec", &O2DProxy::getScaleVec, &O2DProxy::setScaleVec)
            .addProperty("Size", &O2DProxy::getSize, &O2DProxy::setSize)
            .addProperty("size", &O2DProxy::getSize, &O2DProxy::setSize)
            .addProperty("Rect", &O2DProxy::getRect, &O2DProxy::setRect)
            .addProperty("rect", &O2DProxy::getRect, &O2DProxy::setRect)
                    /// Change rotation by this value (in degrees)
                    // @function AddRotation
                    // @param rot The change in rotation.
            .addFunction("AddRotation", &O2DProxy::AddRotation)
                    /// Set scale. Shorthand for ScaleX = ScaleY = param.
                    // @property Scale
                    // @param scale The scale to set both variables to.
            .addProperty("Scale", &O2DProxy::getScale, &O2DProxy::setScale)
                    /// Z. Equivalent to Layer
                    // @property Z
            .addProperty("Z", &O2DProxy::getZ, &O2DProxy::setZ)
                    /// Layer. If lower, will be behind, if higher, will be above. Ranges from 0 to 32.
                    // @property Layer
            .addProperty("Layer", &O2DProxy::getZ, &O2DProxy::setZ)
                    /// Scale in the horizontal direction.
                    // @property ScaleX
            .addProperty("ScaleX", &O2DProxy::getScaleX, &O2DProxy::setScaleX)
                    /// Scale in the vertical direction.
                    // @property ScaleY
            .addProperty("ScaleY", &O2DProxy::getScaleY, &O2DProxy::setScaleY)
                    /// Rotation in degrees - results vary if using Centered.
                    // @property Rotation
            .addProperty("Rotation", &O2DProxy::getRotation, &O2DProxy::setRotation)
                    /// Width. Set automatically when texture is set. Stacks with ScaleX.
                    // @property Width
            .addProperty("Width", &O2DProxy::getWidth, &O2DProxy::setWidth)
                    /// Height. Set automatically when texture is set. Stacks with ScaleY.
                    // @property Height
            .addProperty("Height", &O2DProxy::getHeight, &O2DProxy::setHeight)
                    /// X position. By default, higher goes to the right. 0 is at the left.
                    // @property X
            .addProperty("X", &O2DProxy::getX, &O2DProxy::setX)
                    /// Y position. By default, higher goes to the bottom. 0 is at the top.
                    // @property Y
            .addProperty("Y", &O2DProxy::getY, &O2DProxy::setY)
                    /***
                    Chain transformation. Setting this causes this transformation to be applied after the current sprite's
                    Useful if you want to make this part of another component.
                    */
                    // @property ChainTransformation
            .addProperty("ChainTransformation",
                         &O2DProxy::getChainTransformation<Sprite>,
                         &O2DProxy::setChainTransformation<Sprite>)
            .addProperty("Parent",
                         &O2DProxy::getChainTransformation<Sprite>,
                         &O2DProxy::setChainTransformation<Sprite>)
                    /// Texture to pass to the shader. Path relative to skin directory.
                    // @property Texture
            .addProperty("Texture", GetImage, SetImage) // Special for setting image.
            .endClass();
}