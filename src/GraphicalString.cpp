#include "Global.h"
#include "GraphObject2D.h"
#include "Font.h"
#include "GraphicalString.h"

GraphicalString::GraphicalString()
	: GraphObject2D(false),
	mFont(NULL)
{
}

void GraphicalString::SetFont(Font& _Font)
{
	assert(_Font);
	mFont = &_Font;
}

Font& GraphicalString::GetFont() const
{
	return *mFont;
}

void GraphicalString::SetText(String _Text)
{
	mText = _Text;
}

String GraphicalString::GetText() const
{
	return mText;
}

void GraphicalString::Render()
{
	if (!mFont) return;

	mFont->Render(mText, Vec2(0, 0), GetMatrix());
}