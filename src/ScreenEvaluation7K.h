#ifndef SCREEN_EVALUATION7K_H_
#define SCREEN_EVALUATION7K_H_

#include "Screen.h"

class BitmapFont;
class ScoreKeeper7K;
class SceneEnvironment;

class ScreenEvaluation7K : public Screen
{
	GString DisplayResult;
	ScoreKeeper7K *Score;
public:
	ScreenEvaluation7K();
	void Init(ScoreKeeper7K *Result);
	bool Run(double Delta);
	void Cleanup();
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	void PrintCLIResults(ScoreKeeper7K *result);
};

#endif
