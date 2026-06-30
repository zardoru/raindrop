#include <string>
#include <glm.h>

#include "Font.h"
#include "Logging.h"

Font::Font() :
    Red(1), Green(1), Blue(1), Alpha(1)
{
}

void Font::set_color(const float red, const float green, const float blue)
{
    Red = red;
    Green = green;
    Blue = blue;
}

void Font::set_alpha(const float alpha)
{
    Alpha = alpha;
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