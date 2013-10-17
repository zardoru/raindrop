#ifndef SCREEN_MAINMANU_H_
#define SCREEN_MAINMENU_H_

class ScreenMainMenu : public IScreen
{
public:
	ScreenMainMenu(IScreen *Parent);
	void Init();
	void HandleInput(int32 key, int32 code, bool isMouseInput);
	bool Run (double Delta);
	void Cleanup();
};

#endif