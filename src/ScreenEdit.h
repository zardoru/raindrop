#ifndef SCR_EDIT_H_
#define SCR_EDIT_H_

#include "ScreenGameplay.h"
#include "GuiTextPrompt.h"

class ScreenEdit : public ScreenGameplay
{

	enum 
	{
		Playing,
		Editing
	}EditScreenState;

	GUI::TextPrompt OffsetPrompt, BPMPrompt;
	uint32 CurrentFraction;
	uint32 CurrentTotalFraction; // basically beat snap
	uint32 savedMeasure;
	BitmapFont EditInfo;

	GameObject* HeldObject;

	void IncreaseTotalFraction();
	void DecreaseTotalFraction();

	GameObject &GetObject();

	float YLock;
	enum
	{
		Select,
		Normal,
		Hold
	}Mode; 
	GraphObject2D GhostObject;

	bool  GridEnabled;
	int32 GridCellSize; // ScreenSize / GridCellSize
	// float GridOffset;

	void DecreaseCurrentFraction();
	void IncreaseCurrentFraction();
	void SaveChart();
	void SwitchPreviewMode();
	void InsertMeasure();

	void OnMousePress(KeyType tkey);
	void OnMouseRelease(KeyType tkey);

	void CalculateVerticalLock();
	void RunGhostObject();
	void DrawInformation();
public:
	ScreenEdit (IScreen * Parent);
	void Init(dotcur::Song *Other);
	void StartPlaying(int32 _Measure);
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool Run (double Delta);
	void Cleanup();
};

#endif