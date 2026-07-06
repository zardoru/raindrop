#include <string>
#include <glm.h>
#include <rmath.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "Font.h"
#include "GraphicalString.h"

GraphicalString::GraphicalString()
    : Sprite(false),
    mFont(nullptr)
{
    SetSize(1); // Default size, so it doesn't scale to 0
    SetZ(0);
	mFontHeight = 16;
	mKernScale = 1;
}

void GraphicalString::set_font(Font* font)
{
    mFont = font;
}

Font* GraphicalString::get_font() const
{
    return mFont;
}

void GraphicalString::set_text(const std::string &text)
{
    mText = text;
}

std::string GraphicalString::get_text() const
{
    return mText;
}

float GraphicalString::get_kerning_scale() const
{
	return mKernScale;
}

void GraphicalString::set_kerning_scale(const float ks)
{
	mKernScale = ks;
}

float GraphicalString::get_text_size() const
{
	if (!mFont) return 0.0f;
	return mFont->get_horizontal_length(mText.c_str()) / SDF_SIZE * mFontHeight * mKernScale;
}

void GraphicalString::set_font_size(const float fsize)
{
	mFontHeight = fsize;
}

float GraphicalString::get_font_size() const
{
	return mFontHeight;
}

void GraphicalString::render()
{
    if (!mFont) return;

    renderer::set_scissor(scissor);
    renderer::set_scissor_region(scissor_region.X1, scissor_region.Y1, scissor_region.width(), scissor_region.height());

    mFont->set_color(color.Red, color.Green, color.Blue);
    mFont->set_alpha(alpha);

	float sc = mFontHeight;
    mFont->render(mText, Vec2(0, 0), GetMatrix(), Vec2(mKernScale, sc));
}