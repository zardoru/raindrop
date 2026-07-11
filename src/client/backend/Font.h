#pragma once

#include <glm.h>
#include <rmath.h>

class Font
{
protected:
    ColorRGB color_;
public:
    Font();

    void set_color(float red, float _Green, float blue);
    void set_alpha(float alpha);

    virtual float get_horizontal_length(const char *Text);
    float measure(const std::string &text, float font_size, float kerning_scale = 1.0f);
    float measure(const std::string &text, float font_size);
    virtual void invalidate();
    virtual void render(const std::string &Text, const Vec2 &Position, const Mat4& Transform = Mat4(), const Vec2 &Scale = Vec2(1,1));
};

#define SDF_SIZE 512
