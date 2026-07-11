#include <string>
#include <glm.h>
#include <filesystem>
#include <map>

#include <text_and_file_util.h>
#include "../game/GameState.h"

#include "LuaManager.h"
#include "Transformation.h"
#include "Sprite.h"

#include "BitmapFont.h"

#include "Texture2D.h"

BitmapFont::BitmapFont()
{
	StartingCharacter = 0;
    Font = nullptr;
}

void BitmapFont::load_font_image(std::filesystem::path Location, const Vec2 _CharSize, const Vec2 _CellSize, const Vec2 _RenderSize, const char FontStart)
{
    if (!Font)
        Font = new Texture2D;

    Font->load_file(Location);

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
            CharPosition[Current].set_width(RenderSize.x);
            CharPosition[Current].set_height(RenderSize.y);
            CharPosition[Current].set_z(8);
            Current++;
        }
    }
}

BitmapFont *BitmapFont::from_lua(LuaManager* Lua, std::string TableName)
{
	auto Ret = new BitmapFont();

    Lua->use_array(TableName);
    std::filesystem::path Locat = Lua->get_field_s("Location", GameState::get_instance().get_skin_prefix() + "font.tga");
    int CharWidth = Lua->get_field_i("CharWidth");
    int CharHeight = Lua->get_field_i("CharHeight");
    int CellWidth = Lua->get_field_i("CellWidth");
    int CellHeight = Lua->get_field_i("CellHeight");
    int RenderWidth = Lua->get_field_i("RenderWidth");
    int RenderHeight = Lua->get_field_i("RenderHeight");
    int FontStart = Lua->get_field_i("FontStart");
    Lua->pop();

    Ret->load_font_image(Locat.c_str(), Vec2(CharWidth, CharHeight), Vec2(CellWidth, CellHeight), Vec2(RenderWidth, RenderHeight), FontStart);

    return Ret;
}

void BitmapFont::load_skin_font_image(std::filesystem::path Location, const Vec2 _CharSize, const Vec2 _CellSize, const Vec2 _RenderSize, const char FontStart)
{
    load_font_image(GameState::get_instance().get_skin_file(otoworm::locale::wstring_to_utf8(Location.wstring())),
                  _CharSize, _CellSize, _RenderSize, FontStart);
}
