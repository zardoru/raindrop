#ifndef SCREEN_MAINMENU_H_
#define SCREEN_MAINMENU_H_

#include "GuiButton.h"
#include "Line.h"

class SceneManager;

class ScreenMainMenu : public Screen
{
	GUI::Button PlayBtn, EditBtn, OptionsBtn, ExitBtn;

	SceneManager * Objects;

	GraphObject2D Background;
public:
	ScreenMainMenu(Screen *Parent);
	void Init();
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	void HandleScrollInput(double xOff, double yOff);
	bool Run (double Delta);
	void Cleanup();
};

#endif