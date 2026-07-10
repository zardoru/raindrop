#pragma once

#include <Transformation.h>
#include <rmath.h>

#include "DrawCallSink.h"

class VBO;
class Texture2D;

namespace renderer {
	class Shader;
};

class Drawable2D : public Transformation
{
public:
    virtual ~Drawable2D() {};
    virtual void emit_draw_calls(DrawCallSink &sink);
    // Stub
};

class Sprite : public Drawable2D
{
protected:
    renderer::Shader *m_shader_;
    VBO *uv_buffer_;
private: // Transformations
    void cleanup();
    Texture2D* m_texture_;

    EBlendMode blending_mode_;

    // These crop variables define where to crop the image.

    /*
    * crop_x1 and crop_y1 define the top-left corner of the crop rectangle.
    * crop_x2 and crop_y2 define the bottom-right corner of the crop rectangle.
    *
    * These coordinates are in fractions.
    */

    float mCrop_x1, mCrop_y1;
    float mCrop_x2, mCrop_y2;



    void construct(bool doInitTexture);
protected:
    void update_texture();
    bool should_draw() const;

    bool dirty_texture_;
    bool do_texture_cleanup_;
public:
    bool centered; // 0 for topleft, 1 for center

    bool scissor;

    bool color_invert;
    bool affected_by_lightning;
    bool black_to_transparent; // If enabled, transforms black pixels into transparent pixels.
public:

    Sprite(bool should_init_texture);
    Sprite();
    ~Sprite();

    // color and other transformations
    float alpha;
    ColorRGB color;
    // float Red, Blue, Green;

    AABB scissor_region;


    void set_image(Texture2D* image, bool reset_size = true);
    Texture2D* get_image() const;
    std::string get_image_filename() const;

    virtual void initialize(bool should_init_texture);

    void set_blend_mode(int mode);
    int get_blend_mode() const;

	void set_shader(renderer::Shader *s);
	renderer::Shader *get_shader() const;

    // Cropping
    void set_crop(Vec2 Crop1, Vec2 Crop2);
    void set_crop1(Vec2 Crop1);
    void set_crop2(Vec2 Crop2);
    void set_crop_to_whole_image();
    void set_crop_by_pixels(int32_t x1, int32_t x2, int32_t y1, int32_t y2);

    void emit_draw_calls(DrawCallSink &sink) override;
    virtual void invalidate();

    void bind_texture_vbo() const;
};
