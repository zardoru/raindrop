#include <string>
#include <glm.h>

#include "Font.h"
#include "Logging.h"

Font::Font() :
    color_{1, 1, 1, 1}
{
}

void Font::set_color(const float red, const float green, const float blue)
{
    color_.Red = red;
    color_.Green = green;
    color_.Blue = blue;
}

void Font::set_alpha(const float alpha)
{
    color_.Alpha = alpha;
}

void Font::invalidate()
{
    /* stub */
}

void Font::render(const std::string &Text, const Vec2 &Position, const Mat4& Transform, const Vec2 &Scale)
{
    /* stub */
}

float Font::get_horizontal_length(const char* Text)
{
    return 0; // stub
}

float Font::measure(const std::string &text, const float font_size, const float kerning_scale)
{
    return get_horizontal_length(text.c_str()) / SDF_SIZE * font_size * kerning_scale;
}

float Font::measure(const std::string &text, const float font_size)
{
    return measure(text, font_size, 1.0f);
}
