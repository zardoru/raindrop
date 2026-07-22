#include <string>
#include <text_and_file_util.h>
#include <rmath.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "VBO.h"
#include "Texture2D.h"

void Drawable2D::emit_draw_calls(DrawCallSink &sink)
{
}

Sprite::Sprite(bool should_init_texture) : Drawable2D()
{
    construct(should_init_texture);
}

Sprite::Sprite() : Drawable2D()
{
    construct(true);
}

void Sprite::construct(const bool do_init_texture)
{
    set_crop_to_whole_image();

    blending_mode_ = BLEND_ALPHA;

    color = {1.0, 1.0, 1.0, 1.0};

    centered = false;
    dirty_texture_ = true;
    do_texture_cleanup_ = do_init_texture;

    m_texture_ = nullptr;
    uv_buffer_ = nullptr;
	m_shader_ = nullptr;

    scissor = false;
    scissor_region = AABB ();

    initialize(do_init_texture);
}

void Sprite::initialize(const bool should_init_texture)
{
    uv_buffer_ = nullptr;

	if (should_init_texture)
		update_texture();
	else
		uv_buffer_ = renderer::get_default_texture_buffer();
}

Sprite::~Sprite()
{
    cleanup();
}

void Sprite::set_blend_mode(int mode)
{
    blending_mode_ = (EBlendMode)mode;
}

int Sprite::get_blend_mode() const
{
    return blending_mode_;
}

void Sprite::set_shader(renderer::Shader * s)
{
	m_shader_ = s;
}

renderer::Shader * Sprite::get_shader() const
{
	return m_shader_;
}

void Sprite::set_image(Texture2D* image, const bool reset_size)
{
    if (m_texture_ != image)
    {
        m_texture_ = image;
        if (image)
        {
            if (reset_size)
            {
                set_crop_to_whole_image();
                set_size(image->w, image->h);
            }
        }
    }
}

void Sprite::set_crop_by_pixels(const int32_t x1, const int32_t x2, const int32_t y1, const int32_t y2)
{
    if (m_texture_)
    {
        crop_.X1 = (float)x1 / (float)m_texture_->w;
        crop_.X2 = (float)x2 / (float)m_texture_->w;
        crop_.Y1 = (float)y1 / (float)m_texture_->h;
        crop_.Y2 = (float)y2 / (float)m_texture_->h;

        dirty_texture_ = true;
        update_texture();
    }
}

void Sprite::set_crop_to_whole_image()
{
    crop_ = {0, 0, 1, 1};
    dirty_texture_ = true;
}

void Sprite::set_crop(const Vec2 crop1, const Vec2 crop2)
{
    crop_ = {crop1.x, crop1.y, crop2.x, crop2.y};
    dirty_texture_ = true;
}

void Sprite::set_crop_topleft(const Vec2 crop1)
{
    crop_.X1 = crop1.x;
    crop_.Y1 = crop1.y;
    dirty_texture_ = true;
}

void Sprite::set_crop_bottomright(const Vec2 crop2)
{
    crop_.X2 = crop2.x;
    crop_.Y2 = crop2.y;
    dirty_texture_ = true;
}

void Sprite::set_crop(const AABB &crop)
{
    crop_ = crop;
    dirty_texture_ = true;
}

AABB Sprite::get_crop() const
{
    return crop_;
}

void Sprite::invalidate()
{
    // stub
}

Texture2D* Sprite::get_image() const
{
    return m_texture_;
}

void Sprite::bind_texture_vbo() const
{
    uv_buffer_->bind();
}

std::string Sprite::get_image_filename() const
{
    if (m_texture_)
        return otoworm::locale::wstring_to_utf8(m_texture_->fname.wstring());
    else
        return std::string();
}
