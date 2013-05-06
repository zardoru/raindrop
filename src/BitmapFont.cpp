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
	int HCellCount = Font->w/CellSize.x, VCellCount = Font->h/CellSize.x;
	unsigned int Current = StartingCharacter;

	for (unsigned short y = 0; y < VCellCount; y++)
	{
		for (unsigned short x = 0; x < HCellCount; x++)
		{
			CharPosition[Current].setImage(Font);
			CharPosition[Current].setCropByPixels(x*CellSize.x, (x)*CellSize.x+CharSize.x,  y*CellSize.y, (y)*CellSize.y+CharSize.y);
			CharPosition[Current].width = RenderSize.x;
			CharPosition[Current].height = RenderSize.y;
			Current++;
		}
	}
}

void BitmapFont::DisplayText(const char* Text, glm::vec2 Position)
{
	int Character = 0;
	/* OpenGL Code Ahead */

	for (;*Text != '\0'; Text++)
	{
		CharPosition[*Text].position = glm::vec2(Position.x+Character, Position.y);
		CharPosition[*Text].Render();

		Character += RenderSize.x;
	}

}