#ifndef SCREEN_EVALUATION_H_
#define SCREEN_EVALUATION_H_

#include "Screen.h"

class BitmapFont;

class ScreenEvaluation : public Screen
{
	EvaluationData Results;
	Sprite Background;
	BitmapFont* Font;

	GString ResultsGString, ResultsNumerical;
	GString TitleFormat;
	
	int32 CalculateScore();
public:
	ScreenEvaluation(Screen *Parent);
	void Init(EvaluationData _Data, GString SongAuthor, GString SongTitle);
	bool Run(double Delta);
	void Cleanup();
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif