#ifndef SCREENSELECTMUSIC_H_
#define SCREENSELECTMUSIC_H_

class BitmapFont;

class ScreenSelectMusic : public IScreen
{
	int Cursor, OldCursor;
	GraphObject2D Background, SelCursor, Logo;
	BitmapFont* Font;
	std::vector<Song*> SongList;
	bool SwitchBackGuiPending;
	void StopLoops();
	void UpdateCursor();
	double Time;
	void InitializeObjects();
public:
	ScreenSelectMusic();
	void Init();
	bool Run(double Delta);
	void Cleanup();
	void HandleInput(int32 key, KeyEventType code, bool isMouseInput);
};

#endif