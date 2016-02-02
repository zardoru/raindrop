#include "pch.h"


#include "GameState.h"
#include "LuaManager.h"
#include "Sprite.h"
#include "BitmapFont.h"
#include "ImageLoader.h"
#include "Directory.h"

BitmapFont::BitmapFont()
{
	Font = NULL;
}

void BitmapFont::LoadFontImage(const char* Location, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize, char FontStart)
{
	if (!Font)
		Font = new Image;
	
	Font->Assign(Location, ImageData::SM_NEAREST);


	StartingCharacter = FontStart;
	CharSize = _CharSize;
	CellSize = _CellSize;
	RenderSize = _RenderSize;
	RegenerateCharPositions(CellSize);
}

void BitmapFont::LoadSkinFontImage(const char* Location, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize, char FontStart)
{
	LoadFontImage(GameState::GetInstance().GetSkinFile(Location).c_str(), _CharSize, _CellSize, _RenderSize, FontStart);
}

void BitmapFont::RegenerateCharPositions(Vec2 CellSize)
{
	if (!Font)
		return;

	int32 HCellCount = (int)((float)Font->w / (float)CellSize.x), VCellCount = (int)((float)Font->h/(float)CellSize.y);
	uint32 Current = StartingCharacter;

	for (unsigned short y = 0; y < VCellCount; y++)
	{
		for (unsigned short x = 0; x < HCellCount; x++)
		{
			if (Current >= 256)
				return;

			// Ideally, we should actually be using a texture matrix.
			CharPosition[Current].SetImage(Font);
			CharPosition[Current].SetCropByPixels(x*CellSize.x, (x)*CellSize.x+CharSize.x,  y*CellSize.y, (y)*CellSize.y+CharSize.y);
			CharPosition[Current].SetWidth(RenderSize.x);
			CharPosition[Current].SetHeight(RenderSize.y);
			CharPosition[Current].SetZ(16);
			Current++;
		}
	}
}

void BitmapFont::SetAffectedByLightning(bool Lightning)
{
	for (int i = 0; i < 256; i++)
	{
		CharPosition[i].AffectedByLightning = Lightning;
	}
}

BitmapFont *BitmapFont::FromLua(LuaManager* Lua, GString TableName)
{
	BitmapFont* Ret = new BitmapFont();

	Lua->UseArray(TableName);
	Directory Locat  = Lua->GetFieldS("Location", GameState::GetInstance().GetSkinPrefix() + "font.tga");
	int CharWidth    = Lua->GetFieldI("CharWidth");
	int CharHeight   = Lua->GetFieldI("CharHeight");
	int CellWidth    = Lua->GetFieldI("CellWidth");
	int CellHeight   = Lua->GetFieldI("CellHeight");
	int RenderWidth  = Lua->GetFieldI("RenderWidth");
	int RenderHeight = Lua->GetFieldI("RenderHeight");
	int FontStart    = Lua->GetFieldI("FontStart");
	Lua->Pop();

	Ret->LoadFontImage(Locat.c_path(), Vec2(CharWidth, CharHeight), Vec2(CellWidth, CellHeight), Vec2(RenderWidth, RenderHeight), FontStart);

	return Ret;
}
