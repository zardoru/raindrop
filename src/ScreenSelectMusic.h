#pragma once

class BitmapFont;

namespace dotcur
{
	class Song;
}

namespace VSRG
{
	class Song;
}

namespace GUI
{
	class Button;
}

class SceneEnvironment;
class AudioStream;

class ScreenSelectMusic : public Screen
{
	double Time;
	double TransitionTime;
	double PreviewWaitTime;

	shared_ptr<Game::Song> ToPreview;
	shared_ptr<Game::Song> PreviousPreview;
	Sprite Background;
	BitmapFont* Font;

	GUI::Button *UpBtn, *BackBtn, *AutoBtn;

	shared_ptr<AudioStream> PreviewStream;

	bool SwitchBackGuiPending;

	bool OptionUpscroll;

	bool IsTransitioning;
	
	void PlayPreview();
	void PlayLoops();
	void StopLoops();

	float GetListHorizontalTransformation(const float Y);
	void StartGameplayScreen();
	float GetListVerticalTransformation(const float Y);
	float GetListPendingVerticalTransformation(const float Y);
	void OnSongChange(shared_ptr<Game::Song> MySong, uint8 difindex);
	void OnSongSelect(shared_ptr<Game::Song> MySong, uint8 difindex);

	void OnDirectoryChange();
	void OnItemClick(int32 Index, uint32 boundIndex, GString Line, shared_ptr<Game::Song> Selected);
	void OnItemHover(int32 Index, uint32 boundIndex, GString Line, shared_ptr<Game::Song> Selected);
	void OnItemHoverLeave(int32 Index, uint32 boundIndex, GString Line, shared_ptr<Game::Song> Selected);

	void TransformItem(int Item, shared_ptr<Game::Song> Song, bool IsSelected, int ListItem);
	void TransformString(int Item, shared_ptr<Game::Song> Song, bool IsSelected, int ListItem, GString text);
	void SwitchUpscroll(bool NewUpscroll);
public:
	ScreenSelectMusic();
	void LoadResources() override;
	void InitializeResources() override;
	bool Run(double Delta) override;
	void Cleanup() override;
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput) override;
	bool HandleScrollInput(double xOff, double yOff) override;
};