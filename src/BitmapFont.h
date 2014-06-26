#ifndef BITMAPFONT_H_
#define BITMAPFONT_H_

#include "Image.h"

class LuaManager;

class BitmapFont
{
	Image *Font;
	GraphObject2D CharPosition[258];
	Vec2 CharSize, CellSize, RenderSize;
	unsigned char StartingCharacter;

	void RegenerateCharPositions(Vec2 CellSize);

public:
	BitmapFont();
	void DisplayText(const char* Text, Vec2 Position);
	void LoadFontImage(const char* Name, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize = Vec2(1,1), char FontStart = 0);
	void LoadSkinFontImage(const char* Name, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize = Vec2(1,1), char FontStart = 0);
	void SetAffectedByLightning(bool Lightning);
	BitmapFont *FromLua(LuaManager* Lua, std::string TableName);
};

#endif
