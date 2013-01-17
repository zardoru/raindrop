#ifndef SCREENSELECTMUSIC_H_
#define SCREENSELECTMUSIC_H_

#include <CEGUI.h>

class ScreenSelectMusic : public IScreen
{
	std::vector<Song*> SongList;
	CEGUI::DefaultWindow *root;
	CEGUI::Listbox *songbox;
	bool RunMusic(const CEGUI::EventArgs&);
	bool QuitGame(const CEGUI::EventArgs&);
	bool ReloadSongs(const CEGUI::EventArgs&);
	bool SwitchBackGuiPending;
public:
	ScreenSelectMusic();
	void Init();
	bool Run(float Delta);
	void Cleanup();
	void HandleInput(int key, int code, bool isMouseInput);
};

#endif