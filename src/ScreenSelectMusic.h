#ifndef SCREENSELECTMUSIC_H_
#define SCREENSELECTMUSIC_H_

class BitmapFont;

class ScreenSelectMusic : public IScreen
{
	int Cursor, OldCursor;
	GraphObject2D Background, SelCursor, Logo;
	BitmapFont* Font;

	/* Mode-based song list. */
	std::vector<SongDC*> SongList;
	std::vector<Song7K*> SongList7K;

	bool SwitchBackGuiPending;
	void StopLoops();
	void UpdateCursor();
	double Time;

	ModeType SelectedMode;

public:
	ScreenSelectMusic();
	void LoadThreadInitialization();
	void MainThreadInitialization();
	bool Run(double Delta);
	void Cleanup();
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif