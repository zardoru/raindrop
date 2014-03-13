#ifndef SCREEN_EVALUATION7K_H_
#define SCREEN_EVALUATION7K_H_

#include "Screen.h"

class BitmapFont;

class ScreenEvaluation7K : public IScreen
{
	GraphObject2D Background;
	BitmapFont* Font;

public:
	ScreenEvaluation7K(IScreen *Parent);
	void Init(/*??*/);
	bool Run(double Delta);
	void Cleanup();
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif