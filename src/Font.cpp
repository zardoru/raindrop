#include "Global.h"
#include "Font.h"

Font::Font() :
	Red(1), Green(1), Blue(1), Alpha(1)
{
}

void Font::SetColor(float _Red, float _Green, float _Blue)
{
	Red = _Red;
	Green = _Green;
	Blue = _Blue;
}

void Font::SetAlpha(float _Alpha)
{
	Alpha = _Alpha;
}

void Font::Invalidate()
{
	/* stub */
}

void Font::Render(String Text, const Vec2 &Position, const Mat4& Transform)
{
	/* stub */
}