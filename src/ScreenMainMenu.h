#ifndef SCREEN_MAINMANU_H_
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
	bool Run (double Delta);
	void Cleanup();
};

#endif