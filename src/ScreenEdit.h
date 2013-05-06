#ifndef SCR_EDIT_H_
#define SCR_EDIT_H_

#include <CEGUI.h>
#include "ScreenGameplay.h"

class ScreenEdit : public ScreenGameplay
{
	CEGUI::DefaultWindow *root;
	CEGUI::FrameWindow* fWndInfo;
	enum 
	{
		Playing,
		Editing
	}EditScreenState;

	bool GuiInitialized;
	CEGUI::Editbox *CurrentMeasure;
	CEGUI::Editbox *BPMBox;
	CEGUI::Editbox *FracBox;
	CEGUI::Editbox *OffsetBox;
	bool measureTextChanged(const CEGUI::EventArgs& param);
	bool bpmTextChanged(const CEGUI::EventArgs& param);
	bool offsetTextChanged(const CEGUI::EventArgs& param);
	bool fracTextChanged(const CEGUI::EventArgs& param);
	uint32_t CurrentFraction;
	uint32_t savedMeasure;

	float YLock;
	enum
	{
		Select,
		Normal,
		Hold
	}Mode; 
	GraphObject2D GhostObject;
public:
	ScreenEdit (IScreen * Parent);
	void Init(Song *Other);
	void StartPlaying(int _Measure);
	void HandleInput(int key, int code, bool isMouseInput);
	bool Run (double Delta);
	void Cleanup();
};

#endif