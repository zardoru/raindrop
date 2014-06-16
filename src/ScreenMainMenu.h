#ifndef SCREEN_MAINMENU_H_
#define SCREEN_MAINMENU_H_

#include "GuiButton.h"

class GraphObjectMan;

class ScreenMainMenu : public IScreen
{
	GUI::Button PlayBtn, EditBtn, OptionsBtn, ExitBtn;

	GraphObjectMan * Objects;

	GraphObject2D Background;
public:
	ScreenMainMenu(IScreen *Parent);
	void Init();
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	void HandleScrollInput(double xOff, double yOff);
	bool Run (double Delta);
	void Cleanup();
};

#endif