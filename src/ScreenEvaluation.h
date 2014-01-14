#ifndef SCREEN_EVALUATION_H_
#define SCREEN_EVALUATION_H_

#include "Screen.h"

class BitmapFont;

class ScreenEvaluation : public IScreen
{
	EvaluationData Results;
	GraphObject2D Background;
	BitmapFont* Font;

	String ResultsString, ResultsNumerical;
	String TitleFormat;
	// CEGUI::FrameWindow* wnd;
	int32 CalculateScore();
public:
	ScreenEvaluation(IScreen *Parent);
	void Init(EvaluationData _Data, String SongAuthor, String SongTitle);
	bool Run(double Delta);
	void Cleanup();
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif