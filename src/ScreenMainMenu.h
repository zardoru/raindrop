#ifndef SCREEN_MAINMENU_H_
#define SCREEN_MAINMENU_H_

#include "GuiButton.h"

class ScreenMainMenu : public IScreen
{
	GUI::Button PlayBtn, EditBtn, OptionsBtn, ExitBtn;
	GraphObject2D Logo, Background;
public:
	ScreenMainMenu(IScreen *Parent);
	void Init();
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	void HandleScrollInput(double xOff, double yOff);
	bool Run (double Delta);
	void Cleanup();
};

#endif