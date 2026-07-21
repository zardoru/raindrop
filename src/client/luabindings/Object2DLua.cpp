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
        return obj->get_z();
    }

    static float getScaleX(Sprite const *obj) {
        return obj->get_scale_x();
    }

    static float getScaleY(Sprite const *obj) {
        return obj->get_scale_y();
    }

    static float getWidth(Sprite const *obj) {
        return obj->get_width();
    }

    static float getHeight(Sprite const *obj) {
        return obj->get_height();
    }

    static float getX(Sprite const *obj) {
        return obj->get_position_x();
    }

    static float getY(Sprite const *obj) {
        return obj->get_position_y();
    }

    static float getRotation(Sprite const *obj) {
        return obj->get_rotation();
    }

    static float getRed(Sprite const *obj) {
        return obj->color.red;
    }

    static float getAlpha(Sprite const *obj) {
        return obj->color.alpha;
    }

    static void setAlpha(Sprite *obj, float param) {
        obj->color.alpha = param;
    }

    static void setGreen(Sprite *obj, float param) {
        obj->color.green = param;
    }

    static float getGreen(Sprite const *obj) {
        return obj->color.green;
    }

    static void setBlue(Sprite *obj, float param) {
        obj->color.blue = param;
    }

    static float getBlue(Sprite const *obj) {
        return obj->color.blue;
    }

    static void setRed(Sprite *obj, float param) {
        obj->color.red = param;
    }

    template<class T>
    static Transformation getChainTransformation(T const *obj) {
        return Transformation();
    }

    static void setZ(Sprite *obj, uint32_t nZ) {
        obj->set_z(nZ);
    }

    static void setHeight(Sprite *obj, float param) {
        obj->set_height(param);
    }

    static void setWidth(Sprite *obj, float param) {
        obj->set_width(param);
    }

    static void setScaleY(Sprite *obj, float param) {
        obj->set_scale_y(param);
    }

    static void setScaleX(Sprite *obj, float param) {
        obj->set_scale_x(param);
    }

    static void setRotation(Sprite *obj, float param) {
        obj->set_rotation(param);
    }

    static void setX(Sprite *obj, float param) {
        obj->set_position_x(param);
    }

    static void setY(Sprite *obj, float param) {
        obj->set_position_y(param);
    }

    template<class T>
    static void setChainTransformation(T *obj, Transformation *param) {
        obj->chain_transformation(param);
    }

    static void setScale(Sprite *obj, float param) {
        obj->set_scale(param);
    }

    static void AddRotation(Sprite *obj, float param) {
        obj->add_rotation(param);
    }

    static float getScale(Sprite const *obj) {
        return (obj->get_scale_x() + obj->get_scale_y()) / 2;
    }

    static VectorLua getScaleVec(Sprite const *obj) {
        return {obj->get_scale_x(), obj->get_scale_y()};
    }

    static void setScaleVec(Sprite *obj, VectorLua v) {
        obj->set_scale_x(v.getX());
        obj->set_scale_y(v.getY());
    }

    static VectorLua getSize(Sprite const *obj) {
        return {obj->get_width(), obj->get_height()};
    }

    static void setSize(Sprite *obj, VectorLua v) {
        obj->set_width(v.getX());
        obj->set_height(v.getY());
    }

    static VectorLua getPosition(Sprite const *obj) {
        return {obj->get_position_x(), obj->get_position_y()};
    }

    static void setPosition(Sprite *obj, VectorLua v) {
        obj->set_position_x(v.getX());
        obj->set_position_y(v.getY());
    }

    static AABB getRect(Sprite const *obj) {
        return {
                obj->get_position_x(),
                obj->get_position_y(),
                obj->get_position_x() + obj->get_width(),
                obj->get_position_y() + obj->get_height()
        };
    }

    static void setRect(Sprite *obj, AABB box) {
        obj->set_position_x(box.X1);
        obj->set_position_y(box.X2);
        obj->set_width(box.width());
        obj->set_height(box.height());
    }
};

// Wrapper functions
void SetImage(Sprite *O, std::string dir) {
    O->set_image(GameState::get_instance().get_skin_image(dir));
    if (O->get_image() == nullptr)
        Log::Printf("File %s could not be loaded.\n", dir.c_str());
}

std::string GetImage(const Sprite *O) {
    return O->get_image_filename();
}

void CreateObject2DLua(LuaManager *anim_lua) {
    /// Blend modes allowed by Object2D.
    // @enum BlendMode
    // @param Add Addition blend mode.
    // @param Alpha Alpha blend mode. Default.
    anim_lua->new_array();
    anim_lua->set_field_i("Add", (int) BLEND_ADD);
    anim_lua->set_field_i("Alpha", (int) BLEND_ALPHA);
    anim_lua->finalize_enum("BlendMode");

    ///
    luabridge::getGlobalNamespace(anim_lua->get_lua_state())
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
            .addData("Centered", &Sprite::centered)
            .addData("Scissor", &Sprite::scissor)
            .addData("ScissorRegion", &Sprite::scissor_region)
                    /// The sprite's alpha.
                    // @property Alpha
            .addProperty("Alpha", &O2DProxy::getAlpha, &O2DProxy::setAlpha)
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
            .addProperty("BlendMode", &Sprite::get_blend_mode, &Sprite::set_blend_mode)
                    /// Set the crop of the sprite by pixel measurements.
                    // @function SetCropByPixels
                    // @param x1 Left X coordinate.
                    // @param x2 Right X coordinate.
                    // @param y1 Top Y coordinate.
                    // @param y2 Bottom Y coordinate.
            .addFunction("SetCropByPixels", &Sprite::set_crop_by_pixels)
			/// Set normalized UV crop bounds.
			// @property Crop
			.addProperty("Crop", &Sprite::get_crop, static_cast<void (Sprite::*)(const AABB &)>(&Sprite::set_crop))
                    /// Reset the crop to the whole image.
                    // @function ResetCrop
            .addFunction("ResetCrop", &Sprite::set_crop_to_whole_image)
                    /// @{Shader} to render this sprite with.
                    // @property Shader
            .addProperty("Shader", &Sprite::get_shader, &Sprite::set_shader)
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
                    /// Layer. If lower, will be behind, if higher, will be above. Ranges from 0 to 15.
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
