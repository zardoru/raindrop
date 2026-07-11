#pragma once

#include <glm.h>

class Transformation
{
    static constexpr uint8_t MaxLayer = 15;
    Transformation* chain_;
    Mat4   m_matrix_;
    float  m_width_, m_height_;
    Vec2   m_position_;
    Vec2 m_scale_;
    float m_rotation_;
    bool   m_dirty_matrix_;
    uint8_t m_layer_;

	bool is_matrix_dirty() const;
public:
    Transformation();

    // Scale
    void set_scale(Vec2 scale);
    void set_scale(float scale);
    void set_scale_x(float scale_x);
    void set_scale_y(float scale_y);
    float get_scale_x() const;
    float get_scale_y() const;
    Vec2 get_scale() const;

    // Position
    void set_position(Vec2 pos);
    void set_position(float p_x, float p_y);
    void move(Vec2 pos);
    void move(float p_x, float p_y);
    void set_position_x(float p_x);
    void set_position_y(float p_y);
    void add_position_x(float p_x);
    void add_position_y(float p_y);
    Vec2 get_position() const;
    float get_position_x() const;
    float get_position_y() const;
    void set_z(uint8_t z);
    uint8_t get_z() const;

    // Size
    void set_size(Vec2 size);
    void set_size(float size);
    void set_size(float w, float h);
    void set_width(float w);
    void set_height(float h);
    float get_width() const;
    float get_height() const;
    Vec2 get_size() const;

    // Rotation
    void set_rotation(float rot);
    float get_rotation() const;
    void add_rotation(float rot);

    void chain_transformation(Transformation *other);

    const glm::mat4 &as_matrix();
    void update_matrix();
};
