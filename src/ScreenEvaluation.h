#ifndef SCREEN_EVALUATION_H_
#define SCREEN_EVALUATION_H_

#include "Screen.h"
#include <CEGUI.h>

class ScreenEvaluation : public IScreen
{
	EvaluationData Results;
	CEGUI::DefaultWindow* root;
	// CEGUI::FrameWindow* wnd;
	int CalculateScore();
	bool StopRunning(const CEGUI::EventArgs&);
public:
	ScreenEvaluation(IScreen *Parent);
	void Init(EvaluationData _Data);
	bool Run(float Delta);
	void Cleanup();
	void HandleInput(int key, int code, bool isMouseInput);
};

#endif