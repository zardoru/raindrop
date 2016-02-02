#pragma once

#include "Screen.h"

class BitmapFont;

class ScreenEvaluation : public Screen
{
	EvaluationData Results;
	Sprite Background;
	BitmapFont* Font;

	GString ResultsGString, ResultsNumerical;
	GString TitleFormat;
	
	int32_t CalculateScore();
public:
	ScreenEvaluation();
	void Init(EvaluationData _Data, GString SongAuthor, GString SongTitle);
	bool Run(double Delta) override;
	void Cleanup() override;
	bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput) override;
};