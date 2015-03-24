#ifndef SCREENSELECTMUSIC_H_
#define SCREENSELECTMUSIC_H_

#include <boost/thread.hpp>

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

	SceneEnvironment *Objects;

	GUI::Button *UpBtn, *BackBtn, *AutoBtn;

	AudioStream *PreviewStream;

	bool SwitchBackGuiPending;

	bool OptionUpscroll;

	bool IsTransitioning;
	
	void PlayPreview();
	void PlayLoops();
	void StopLoops();

	float GetListHorizontalTransformation(const float Y);
	float GetListVerticalTransformation(const float Y);
	float GetListPendingVerticalTransformation(const float Y);
	void OnSongChange(shared_ptr<Game::Song> MySong, uint8 difindex);
	void OnSongSelect(shared_ptr<Game::Song> MySong, uint8 difindex);

	void OnDirectoryChange();
	void OnItemClick(uint32 Index, GString Line, shared_ptr<Game::Song> Selected);
	void OnItemHover(uint32 Index, GString Line, shared_ptr<Game::Song> Selected);
	void OnItemHoverLeave(uint32 Index, GString Line, shared_ptr<Game::Song> Selected);

	void TransformItem(Sprite* Item, shared_ptr<Game::Song> Song, bool IsSelected);
	void SwitchUpscroll(bool NewUpscroll);
public:
	ScreenSelectMusic();
	void LoadThreadInitialization();
	void MainThreadInitialization();
	bool Run(double Delta);
	void Cleanup();
	bool HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	bool HandleScrollInput(double xOff, double yOff);
};

#endif