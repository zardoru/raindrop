#ifndef SCREEN_EVALUATION7K_H_
#define SCREEN_EVALUATION7K_H_

#include "Screen.h"

class BitmapFont;
class ScoreKeeper7K;

class ScreenEvaluation7K : public Screen
{
	GraphObject2D Background;
	BitmapFont* Font;
	GString DisplayResult;
	ScoreKeeper7K *Score;

public:
	ScreenEvaluation7K(Screen *Parent);
	void Init(ScoreKeeper7K *Result);
	bool Run(double Delta);
	void Cleanup();
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif