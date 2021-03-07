#include <string>
#include <glm.h>
#include <filesystem>
#include <map>

#include <rmath.h>

#include <game/GameConstants.h>
#include <TextAndFileUtil.h>
#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"

#include "LuaManager.h"
#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "Font.h"
#include "BitmapFont.h"

#include "Texture.h"

BitmapFont::BitmapFont()
{
	StartingCharacter = 0;
    Font = nullptr;
}

void BitmapFont::LoadFontImage(std::filesystem::path Location, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize, char FontStart)
{
    if (!Font)
        Font = new Texture;

    Font->LoadFile(Location);

    StartingCharacter = FontStart;
    CharSize = _CharSize;
    CellSize = _CellSize;
    RenderSize = _RenderSize;
    RegenerateCharPositions(CellSize);
}



void BitmapFont::RegenerateCharPositions(Vec2 CellSize)
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
            CharPosition[Current].SetImage(Font);
            CharPosition[Current].SetCropByPixels(x*CellSize.x, (x)*CellSize.x + CharSize.x, y*CellSize.y, (y)*CellSize.y + CharSize.y);
            CharPosition[Current].SetWidth(RenderSize.x);
            CharPosition[Current].SetHeight(RenderSize.y);
            CharPosition[Current].SetZ(16);
            Current++;
        }
    }
}

BitmapFont *BitmapFont::FromLua(LuaManager* Lua, std::string TableName)
{
	auto Ret = new BitmapFont();

    Lua->UseArray(TableName);
    std::filesystem::path Locat = Lua->GetFieldS("Location", GameState::GetInstance().GetSkinPrefix() + "font.tga");
    int CharWidth = Lua->GetFieldI("CharWidth");
    int CharHeight = Lua->GetFieldI("CharHeight");
    int CellWidth = Lua->GetFieldI("CellWidth");
    int CellHeight = Lua->GetFieldI("CellHeight");
    int RenderWidth = Lua->GetFieldI("RenderWidth");
    int RenderHeight = Lua->GetFieldI("RenderHeight");
    int FontStart = Lua->GetFieldI("FontStart");
    Lua->Pop();

    Ret->LoadFontImage(Locat.c_str(), Vec2(CharWidth, CharHeight), Vec2(CellWidth, CellHeight), Vec2(RenderWidth, RenderHeight), FontStart);

    return Ret;
}

void BitmapFont::LoadSkinFontImage(std::filesystem::path Location, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize, char FontStart)
{
    LoadFontImage(GameState::GetInstance().GetSkinFile(Conversion::ToU8(Location.wstring())),
                  _CharSize, _CellSize, _RenderSize, FontStart);
}