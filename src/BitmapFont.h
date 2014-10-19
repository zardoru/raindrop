#ifndef BITMAPFONT_H_
#define BITMAPFONT_H_

#include "Font.h"
#include "Image.h"

class LuaManager;

class BitmapFont : public Font
{
	Image *Font;
	GraphObject2D CharPosition[258];
	Vec2 CharSize, CellSize, RenderSize;
	unsigned char StartingCharacter;

	void RegenerateCharPositions(Vec2 CellSize);

public:
	BitmapFont();
	void Render(String Text, const Vec2 &Position, const Mat4& Transform = Mat4());
	void LoadFontImage(const char* Name, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize = Vec2(1,1), char FontStart = 0);
	void LoadSkinFontImage(const char* Name, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize = Vec2(1,1), char FontStart = 0);
	void SetAffectedByLightning(bool Lightning);
	BitmapFont *FromLua(LuaManager* Lua, std::string TableName);
};

#endif
