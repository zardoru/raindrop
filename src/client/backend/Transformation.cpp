#include "Transformation.h"
//#include <glm/gtc/matrix_transform.inl>

#include <algorithm>

bool Transformation::is_matrix_dirty() const {
	return m_dirty_matrix_ || chain_;
}

Transformation::Transformation()
{
    set_size(1);
    set_scale(1);
    set_rotation(0);
    set_position(0, 0);
    chain_ = nullptr;
    m_layer_ = 0;
	m_dirty_matrix_ = true;

    update_matrix();
}

// Scale
void Transformation::set_scale(const Vec2 scale)
{
    m_scale_ = scale;
    m_dirty_matrix_ = true;
}

void Transformation::set_scale(const float scale)
{
    set_scale_x(scale);
    set_scale_y(scale);
}

void Transformation::set_scale_x(const float scale_x)
{
    m_scale_.x = scale_x;
    m_dirty_matrix_ = true;
}

void Transformation::set_scale_y(const float scale_y)
{
    m_scale_.y = scale_y;
    m_dirty_matrix_ = true;
}

Vec2 Transformation::get_scale() const
{
    return m_scale_;
}

// Position
void Transformation::set_position(const Vec2 pos)
{
    m_position_ = pos;
    m_dirty_matrix_ = true;
}

void Transformation::set_position(const float p_x, const float p_y)
{
    set_position_x(p_x);
    set_position_y(p_y);
}

void Transformation::set_position_x(const float p_x)
{
    m_position_.x = p_x;
    m_dirty_matrix_ = true;
}

void Transformation::set_position_y(const float p_y)
{
    m_position_.y = p_y;
    m_dirty_matrix_ = true;
}

void Transformation::move(const float p_x, const float p_y)
{
    m_position_.x += p_x;
    m_position_.y += p_y;
    m_dirty_matrix_ = true;
}

void Transformation::move(const Vec2 pos)
{
    m_position_ += pos;
    m_dirty_matrix_ = true;
}

void Transformation::add_position_x(const float p_x)
{
    move(p_x, 0);
}

void Transformation::add_position_y(const float p_y)
{
    move(0, p_y);
}

Vec2 Transformation::get_position() const
{
    return m_position_;
}

// Size
void Transformation::set_size(const Vec2 size)
{
    m_width_ = size.x;
    m_height_ = size.y;
    m_dirty_matrix_ = true;
}

void Transformation::set_size(const float size)
{
    set_size(Vec2(size, size));
}

void Transformation::set_size(const float w, const float h)
{
    set_size(Vec2(w, h));
}

void Transformation::set_width(const float w)
{
    m_width_ = w;
    m_dirty_matrix_ = true;
}

void Transformation::set_height(const float h)
{
    m_height_ = h;
    m_dirty_matrix_ = true;
}

Vec2 Transformation::get_size() const
{
    return Vec2(m_width_, m_height_);
}

float Transformation::get_width() const
{
    return m_width_;
}

float Transformation::get_height() const
{
    return m_height_;
}

// Rotation
void Transformation::set_rotation(const float rot)
{
    m_rotation_ = rot;
    m_dirty_matrix_ = true;
}

float Transformation::get_rotation() const
{
    return m_rotation_;
}

void Transformation::add_rotation(const float rot)
{
    m_rotation_ += rot;

    if (m_rotation_ >= 360)
        m_rotation_ -= 360;
    m_dirty_matrix_ = true;
}

uint8_t Transformation::get_z() const
{
    return m_layer_;
}

void Transformation::set_z(const uint8_t z)
{
    m_layer_ = std::min(z, MaxLayer);
    m_dirty_matrix_ = true;
}

const glm::mat4 &Transformation::as_matrix()
{
    if (is_matrix_dirty())
        update_matrix();

    return m_matrix_;
}

float Transformation::get_scale_x() const
{
    return m_scale_.x;
}

float Transformation::get_scale_y() const
{
    return m_scale_.y;
}

float Transformation::get_position_x() const
{
    return m_position_.x;
}

float Transformation::get_position_y() const
{
    return m_position_.y;
}

void Transformation::update_matrix()
{
    const Mat4 scl = glm::scale(glm::identity<Mat4>(), glm::vec3(m_width_*m_scale_.x, m_height_*m_scale_.y, 1));
    const Mat4 pos = glm::translate(glm::identity<Mat4>(), glm::vec3(m_position_.x, m_position_.y, m_layer_));
    const Mat4 rot = glm::rotate(glm::identity<Mat4>(), m_rotation_, glm::vec3(0, 0, 1));
    auto chn = glm::identity<Mat4>();

    if (chain_)
        chn = chain_->as_matrix();

    m_matrix_ = chn * pos * rot * scl;
    m_dirty_matrix_ = false;
}

void Transformation::chain_transformation(Transformation *other)
{
    if (other != this)
        chain_ = other;
}
