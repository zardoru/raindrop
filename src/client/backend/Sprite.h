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
public:

    Sprite(bool should_init_texture);
    Sprite();
    ~Sprite() override;

    void set_image(Texture2D* image, bool reset_size = true);
    Texture2D* get_image() const;
    std::string get_image_filename() const;

    virtual void initialize(bool should_init_texture);

    void set_blend_mode(int mode);
    int get_blend_mode() const;

    void set_shader(renderer::Shader *s);
    renderer::Shader *get_shader() const;

    // Cropping
    void set_crop(Vec2 crop1, Vec2 crop2);
    void set_crop(const AABB &crop);
    AABB get_crop() const;
    void set_crop_topleft(Vec2 crop1);
    void set_crop_bottomright(Vec2 crop2);
    void set_crop_to_whole_image();
    void set_crop_by_pixels(int32_t x1, int32_t x2, int32_t y1, int32_t y2);

    void emit_draw_calls(DrawCallSink &sink) override;
    void emit_draw_calls(DrawCallSink &sink, renderer::Shader *shader_override);
    virtual void invalidate();

    void bind_texture_vbo() const;

private:
    void construct(bool do_init_texture);
    void cleanup();

protected:
    void update_texture();
    bool should_draw(const renderer::Shader *shader = nullptr) const;

    // these are so we can arrange the layout easily
protected:
    renderer::Shader *m_shader_;
    VBO *uv_buffer_;
private: // Transformations

    Texture2D* m_texture_;

public:
    // color and other transformations
    ColorRGBA color;

    AABB scissor_region;
private:

    EBlendMode blending_mode_;

    // Normalized UV crop bounds (top-left to bottom-right).
    AABB crop_;

protected:
    bool dirty_texture_;
    bool do_texture_cleanup_;
public:
    bool centered; // 0 for topleft, 1 for center

    bool scissor;

};
