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

class GraphObjectMan;

class ScreenSelectMusic : public Screen
{
	double Time;
	double TransitionTime;

	GraphObject2D Background;
	BitmapFont* Font;

	GraphObjectMan *Objects;

	GUI::Button *UpBtn, *BackBtn, *AutoBtn;

	bool SwitchBackGuiPending;

	bool OptionUpscroll;

	bool IsTransitioning;
	
	void StopLoops();

	float GetListHorizontalTransformation(const float Y);
	float GetListVerticalTransformation(const float Y);
	float GetListPendingVerticalTransformation(const float Y);
	void OnSongChange(Game::Song* MySong, uint8 difindex);
	void OnSongSelect(Game::Song* MySong, uint8 difindex);

	void OnDirectoryChange();
	void OnItemClick(uint32 Index, GString Line, Game::Song* Selected);
	void OnItemHover(uint32 Index, GString Line, Game::Song* Selected);

	void TransformItem(GraphObject2D* Item, Game::Song* Song, bool IsSelected);
	void SwitchUpscroll(bool NewUpscroll);
public:
	ScreenSelectMusic();
	void LoadThreadInitialization();
	void MainThreadInitialization();
	bool Run(double Delta);
	void Cleanup();
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
	void HandleScrollInput(double xOff, double yOff);
};

#endif