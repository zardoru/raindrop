#ifndef BITMAPFONT_H_
#define BITMAPFONT_H_

#include "Image.h"

class BitmapFont
{
	Image *Font;
	GraphObject2D CharPosition[258];
	glm::vec2 CharSize, CellSize, RenderSize;
	char StartingCharacter;

	void RegenerateCharPositions(glm::vec2 CellSize);

public:
	void DisplayText(const char* Text, glm::vec2 Position);
	void LoadFontImage(char* Name, glm::vec2 _CharSize, glm::vec2 _CellSize, glm::vec2 _RenderSize = glm::vec2(1,1), char FontStart = 0);
	void LoadSkinFontImage(char* Name, glm::vec2 _CharSize, glm::vec2 _CellSize, glm::vec2 _RenderSize = glm::vec2(1,1), char FontStart = 0);
};

#endif