#include <string>
#include <glm.h>
#include <filesystem>
#include <map>

#include <TextAndFileUtil.h>
#include "../game/GameState.h"

#include "LuaManager.h"
#include "Transformation.h"
#include "Sprite.h"

#include "BitmapFont.h"

#include "Texture.h"

BitmapFont::BitmapFont()
{
	StartingCharacter = 0;
    Font = nullptr;
}

void BitmapFont::load_font_image(std::filesystem::path Location, const Vec2 _CharSize, const Vec2 _CellSize, const Vec2 _RenderSize, const char FontStart)
{
    if (!Font)
        Font = new Texture;

    Font->LoadFile(Location);

    StartingCharacter = FontStart;
    CharSize = _CharSize;
    CellSize = _CellSize;
    RenderSize = _RenderSize;
    regenerate_char_positions(CellSize);
}



void BitmapFont::regenerate_char_positions(const Vec2 CellSize)
{
    if (!Font)
        return;

    auto HCellCount = int(float(Font->w) / float(CellSize.x)), VCellCount = int(float(Font->h) / float(CellSize.y));
    uint32_t Current = StartingCharacter;

    for (unsigned short y = 0; y < VCellCount; y++)
    {
        for (unsigned short x = 0; x < HCellCount; x++)
        {
            if (Current >= 256)
                return;

            // Ideally, we should actually be using a texture matrix.
            CharPosition[Current].set_image(Font);
            CharPosition[Current].set_crop_by_pixels(x*CellSize.x, (x)*CellSize.x + CharSize.x, y*CellSize.y, (y)*CellSize.y + CharSize.y);
            CharPosition[Current].SetWidth(RenderSize.x);
            CharPosition[Current].SetHeight(RenderSize.y);
            CharPosition[Current].SetZ(16);
            Current++;
        }
    }
}

BitmapFont *BitmapFont::from_lua(LuaManager* Lua, std::string TableName)
{
	auto Ret = new BitmapFont();

    Lua->UseArray(TableName);
    std::filesystem::path Locat = Lua->GetFieldS("Location", GameState::get_instance().get_skin_prefix() + "font.tga");
    int CharWidth = Lua->GetFieldI("CharWidth");
    int CharHeight = Lua->GetFieldI("CharHeight");
    int CellWidth = Lua->GetFieldI("CellWidth");
    int CellHeight = Lua->GetFieldI("CellHeight");
    int RenderWidth = Lua->GetFieldI("RenderWidth");
    int RenderHeight = Lua->GetFieldI("RenderHeight");
    int FontStart = Lua->GetFieldI("FontStart");
    Lua->Pop();

    Ret->load_font_image(Locat.c_str(), Vec2(CharWidth, CharHeight), Vec2(CellWidth, CellHeight), Vec2(RenderWidth, RenderHeight), FontStart);

    return Ret;
}

void BitmapFont::load_skin_font_image(std::filesystem::path Location, const Vec2 _CharSize, const Vec2 _CellSize, const Vec2 _RenderSize, const char FontStart)
{
    load_font_image(GameState::get_instance().get_skin_file(Conversion::ToU8(Location.wstring())),
                  _CharSize, _CellSize, _RenderSize, FontStart);
}