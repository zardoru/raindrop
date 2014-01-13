#include "Global.h"
#include "LuaManager.h"
#include "GraphObject2D.h"
#include "BitmapFont.h"
#include "ImageLoader.h"
#include "FileManager.h"
#include "Directory.h"

void BitmapFont::LoadFontImage(const char* Location, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize, char FontStart)
{
	Font = ImageLoader::Load(Location);
	StartingCharacter = FontStart;
	CharSize = _CharSize;
	CellSize = _CellSize;
	RenderSize = _RenderSize;
	RegenerateCharPositions(CellSize);
}

void BitmapFont::LoadSkinFontImage(const char* Location, Vec2 _CharSize, Vec2 _CellSize, Vec2 _RenderSize, char FontStart)
{
	Font = ImageLoader::LoadSkin(Location);
	StartingCharacter = FontStart;
	CharSize = _CharSize;
	CellSize = _CellSize;
	RenderSize = _RenderSize;
	RegenerateCharPositions(CellSize);
}

void BitmapFont::RegenerateCharPositions(Vec2 CellSize)
{
	int32 HCellCount = Font->w/CellSize.x, VCellCount = Font->h/CellSize.y;
	uint32 Current = StartingCharacter;

	for (unsigned short y = 0; y < VCellCount; y++)
	{
		for (unsigned short x = 0; x < HCellCount; x++)
		{
			CharPosition[Current].SetImage(Font);
			CharPosition[Current].SetCropByPixels(x*CellSize.x, (x)*CellSize.x+CharSize.x,  y*CellSize.y, (y)*CellSize.y+CharSize.y);
			CharPosition[Current].SetWidth(RenderSize.x);
			CharPosition[Current].SetHeight(RenderSize.y);
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

BitmapFont *BitmapFont::FromLua(LuaManager* Lua, std::string TableName)
{
	BitmapFont* Ret = new BitmapFont();

	Lua->UseArray(TableName);
	Directory Locat  = Lua->GetFieldS("Location", FileManager::GetSkinPrefix() + "font.tga");
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
