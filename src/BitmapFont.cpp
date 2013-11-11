#include "Global.h"
#include "GraphObject2D.h"
#include "BitmapFont.h"
#include "ImageLoader.h"

void BitmapFont::LoadFontImage(char* Location, glm::vec2 _CharSize, glm::vec2 _CellSize, glm::vec2 _RenderSize, char FontStart)
{
	Font = ImageLoader::Load(Location);
	StartingCharacter = FontStart;
	CharSize = _CharSize;
	CellSize = _CellSize;
	RenderSize = _RenderSize;
	RegenerateCharPositions(CellSize);
}

void BitmapFont::LoadSkinFontImage(char* Location, glm::vec2 _CharSize, glm::vec2 _CellSize, glm::vec2 _RenderSize, char FontStart)
{
	Font = ImageLoader::LoadSkin(Location);
	StartingCharacter = FontStart;
	CharSize = _CharSize;
	CellSize = _CellSize;
	RenderSize = _RenderSize;
	RegenerateCharPositions(CellSize);
}

void BitmapFont::RegenerateCharPositions(glm::vec2 CellSize)
{
	int32 HCellCount = Font->w/CellSize.x, VCellCount = Font->h/CellSize.x;
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

void BitmapFont::DisplayText(const char* Text, glm::vec2 Position)
{
	int32 Character = 0, Line = 0;
	/* OpenGL Code Ahead */

	for (;*Text != '\0'; Text++)
	{
		if(*Text == '\n')
		{
			Character = 0;
			Line += RenderSize.y;
			continue;
		}

		if (!Font->IsValid)
		{
			for (int i = 0; i < 256; i++)
			{
				CharPosition[i].Invalidate();
				CharPosition[i].Initialize(true);
			}
		}

		CharPosition[*Text].SetPosition(Position.x+Character, Position.y+Line);
		CharPosition[*Text].Render();

		Character += RenderSize.x;
	}

}

void BitmapFont::SetAffectedByLightning(bool Lightning)
{
	for (int i = 0; i < 256; i++)
	{
		CharPosition[i].AffectedByLightning = Lightning;
	}
}