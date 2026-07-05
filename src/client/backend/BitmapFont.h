#pragma once
#include "Font.h"

class LuaManager;

class BitmapFont : public Font
{
    Texture2D *Font;
    Sprite CharPosition[258];
    Vec2 CharSize, CellSize, RenderSize;
    unsigned char StartingCharacter;

    void regenerate_char_positions(Vec2 CellSize);

public:
    BitmapFont();
    void render(const std::string &Text, const Vec2 &Position, const Mat4& Transform = Mat4(), const Vec2 &Scale = Vec2(1,1)) override;
    void load_font_image(std::filesystem::path Name, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize = Vec2(1, 1), char FontStart = 0);
    void load_skin_font_image(std::filesystem::path Name, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize = Vec2(1, 1), char FontStart = 0);
	static BitmapFont *from_lua(LuaManager* Lua, std::string TableName);
};
