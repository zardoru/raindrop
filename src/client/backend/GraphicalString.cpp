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

void GraphicalString::SetFont(Font* _Font)
{
    mFont = _Font;
}

Font* GraphicalString::GetFont() const
{
    return mFont;
}

void GraphicalString::SetText(std::string _Text)
{
    mText = _Text;
}

std::string GraphicalString::GetText() const
{
    return mText;
}

float GraphicalString::GetKerningScale() const
{
	return mKernScale;
}

void GraphicalString::SetKerningScale(const float ks)
{
	mKernScale = ks;
}

float GraphicalString::GetTextSize() const
{
	if (!mFont) return 0.0f;
	return mFont->get_horizontal_length(mText.c_str()) / SDF_SIZE * mFontHeight * mKernScale;
}

void GraphicalString::SetFontSize(const float fsize)
{
	mFontHeight = fsize;
}

float GraphicalString::GetFontSize() const
{
	return mFontHeight;
}

void GraphicalString::Render()
{
    if (!mFont) return;

    renderer::set_scissor(Scissor);
    renderer::set_scissor_region(ScissorRegion.X1, ScissorRegion.Y1, ScissorRegion.width(), ScissorRegion.height());

    mFont->set_color(Color.Red, Color.Green, Color.Blue);
    mFont->set_alpha(Alpha);

	float sc = mFontHeight;
    mFont->render(mText, Vec2(0, 0), GetMatrix(), Vec2(mKernScale, sc));
}