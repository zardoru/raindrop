#ifndef SCR_EDIT_H_
#define SCR_EDIT_H_

#include <CEGUI.h>
#include "ScreenGameplay.h"

class ScreenEdit : public ScreenGameplay
{
	CEGUI::DefaultWindow *root;
	enum 
	{
		Playing,
		Editing
	}EditScreenState;
public:
	ScreenEdit (IScreen * Parent);
	void Init(Song &Other = Song());
	void StartPlaying(int _Measure);
	void HandleInput(int key, int code, bool isMouseInput);
	bool Run (float Delta);
	void Cleanup();
};

#endif