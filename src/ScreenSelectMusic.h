#ifndef SCREENSELECTMUSIC_H_
#define SCREENSELECTMUSIC_H_

class BitmapFont;

namespace dotcur
{
	class Song;
}

namespace VSRG
{
	class Song;
}

class GraphObjectMan;

class ScreenSelectMusic : public Screen
{
	GraphObject2D Background;
	BitmapFont* Font;

	GraphObjectMan *Objects;

	bool SwitchBackGuiPending;

	bool OptionUpscroll;

	void StopLoops();
	double Time;

	ModeType SelectedMode;

	float GetListYTransformation(const float Y);
	void OnSongChange(Game::Song* MySong, uint8 difindex);
	void OnSongSelect(Game::Song* MySong, uint8 difindex);

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