#pragma once

#include "GuiButton.h"
#include "Line.h"

class SceneEnvironment;

class ScreenMainMenu : public Screen
{
	GUI::Button PlayBtn, EditBtn, OptionsBtn, ExitBtn;

	Screen *TNext;
	Sprite Background;
public:
	ScreenMainMenu();
	void Init();
	bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput);
	bool HandleScrollInput(double xOff, double yOff);

	void OnExitEnd();

	bool Run (double Delta);
	void Cleanup();
};