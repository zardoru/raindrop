#include <string>
#include <text_and_file_util.h>
#include <rmath.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "VBO.h"
#include "Texture2D.h"

void Drawable2D::render() {}

Sprite::Sprite(bool should_init_texture) : Drawable2D()
{
    construct(should_init_texture);
}

Sprite::Sprite() : Drawable2D()
{
    construct(true);
}

void Sprite::construct(const bool doInitTexture)
{
    set_crop_to_whole_image();

    black_to_transparent = false;

    blending_mode_ = BLEND_ALPHA;

    color.Red = color.Blue = color.Green = 1.0;
    alpha = 1.0;

    centered = false;
    color_invert = false;
    dirty_texture_ = true;
    do_texture_cleanup_ = doInitTexture;
    affected_by_lightning = false;

    m_texture_ = nullptr;
    uv_buffer_ = nullptr;
	m_shader_ = nullptr;

    scissor = false;
    scissor_region = AABB ();

    initialize(doInitTexture);
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
                SetSize(image->w, image->h);
            }
        }
    }
}

void Sprite::set_crop_by_pixels(const int32_t x1, const int32_t x2, const int32_t y1, const int32_t y2)
{
    if (m_texture_)
    {
        mCrop_x1 = (float)x1 / (float)m_texture_->w;
        mCrop_x2 = (float)x2 / (float)m_texture_->w;
        mCrop_y1 = (float)y1 / (float)m_texture_->h;
        mCrop_y2 = (float)y2 / (float)m_texture_->h;

        dirty_texture_ = true;
        update_texture();
    }
}

void Sprite::set_crop_to_whole_image()
{
    mCrop_x1 = 0;
    mCrop_x2 = 1;
    mCrop_y1 = 0;
    mCrop_y2 = 1;
    dirty_texture_ = true;
}

void Sprite::set_crop(const Vec2 Crop1, const Vec2 Crop2)
{
    mCrop_x1 = Crop1.x;
    mCrop_y1 = Crop1.y;
    mCrop_x2 = Crop2.x;
    mCrop_y2 = Crop2.y;
    dirty_texture_ = true;
}

void Sprite::set_crop1(const Vec2 Crop1)
{
    mCrop_x1 = Crop1.x;
    mCrop_y1 = Crop1.y;
    dirty_texture_ = true;
}

void Sprite::set_crop2(const Vec2 Crop2)
{
    mCrop_x2 = Crop2.x;
    mCrop_y2 = Crop2.y;
    dirty_texture_ = true;
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
