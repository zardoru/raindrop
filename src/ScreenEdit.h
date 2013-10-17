#ifndef SCR_EDIT_H_
#define SCR_EDIT_H_

#include "ScreenGameplay.h"

class ScreenEdit : public ScreenGameplay
{

	enum 
	{
		Playing,
		Editing
	}EditScreenState;

	bool GuiInitialized;
	uint32_t CurrentFraction;
	uint32_t savedMeasure;
	BitmapFont EditInfo;

	GameObject* HeldObject;

	void AssignFraction(int Measure, int Fraction);

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
	void StartPlaying(int32 _Measure);
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool Run (double Delta);
	void Cleanup();
};

#endif