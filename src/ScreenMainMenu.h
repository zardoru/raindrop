#ifndef SCREEN_MAINMENU_H_
#define SCREEN_MAINMENU_H_

#include "GuiButton.h"
#include "Line.h"

class SceneEnvironment;

class ScreenMainMenu : public Screen
{
	GUI::Button PlayBtn, EditBtn, OptionsBtn, ExitBtn;

	SceneEnvironment * Objects;

	Screen *TNext;
	Sprite Background;
public:
	ScreenMainMenu(Screen *Parent);
	void Init();
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool HandleScrollInput(double xOff, double yOff);

	bool RunIntro(float Fraction);
	bool RunExit(float Fraction);

	bool Run (double Delta);
	void Cleanup();
};

#endif