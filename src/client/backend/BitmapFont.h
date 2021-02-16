#pragma once

class LuaManager;

class BitmapFont : public Font
{
    Texture *Font;
    Sprite CharPosition[258];
    Vec2 CharSize, CellSize, RenderSize;
    unsigned char StartingCharacter;

    void RegenerateCharPositions(Vec2 CellSize);

public:
    BitmapFont();
    void Render(const std::string &Text, const Vec2 &Position, const Mat4& Transform = Mat4(), const Vec2 &Scale = Vec2(1,1)) override;
    void LoadFontImage(std::filesystem::path Name, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize = Vec2(1, 1), char FontStart = 0);
    void LoadSkinFontImage(std::filesystem::path Name, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize = Vec2(1, 1), char FontStart = 0);
    void SetAffectedByLightning(bool Lightning);
	static BitmapFont *FromLua(LuaManager* Lua, std::string TableName);
};
