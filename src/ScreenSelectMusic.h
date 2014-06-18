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

class ScreenSelectMusic : public IScreen
{
	int Cursor, OldCursor;
	GraphObject2D Background, SelCursor, Logo;
	BitmapFont* Font;

	GraphObjectMan *Objects;

	/* Mode-based song list. */
	std::vector<dotcur::Song*> SongList;
	std::vector<VSRG::Song*> SongList7K;

	bool SwitchBackGuiPending;

	bool OptionUpscroll;

	void StopLoops();
	void UpdateCursor();
	double Time;

	float ListY;
	float PendingListY;

	ModeType SelectedMode;

	int diff_index;

	float GetListYTransformation(const float Y);

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