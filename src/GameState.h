#ifndef GAMESTATE_H_
#define GAMESTATE_H_

class GameWindow;

namespace dotcur
{
	class Song;
}

namespace VSRG
{
	struct Difficulty;
	class Song;
}

class SongDatabase;
class Image;

namespace Game
{
class GameState
{
	GString CurrentSkin;
	SongDatabase* Database;

public:

	GameState();

	static void GetSongListDC(std::vector<dotcur::Song*> &OutVec, Directory Dir);
	static void GetSongList7K(std::vector<VSRG::Song*> &OutVec, Directory Dir);

	static GameState &GetInstance();
	void Initialize();

	GString GetDirectoryPrefix();
	GString GetSkinPrefix();
	GString GetScriptsDirectory();
	void SetSkin(GString NextSkin);
	Image* GetSkinImage(Directory Image);
	bool SkinSupportsChannelCount(int Count);

	SongDatabase* GetSongDatabase();
	static GameWindow* GetWindow();
};
}

using Game::GameState;


// This loads the meta only from the database.
void LoadSong7KFromDir( Directory songPath, std::vector<VSRG::Song*> &VecOut );

// This loads the whole of the song.
VSRG::Song* LoadSong7KFromFilename(Directory Filename, Directory Prefix, VSRG::Song *Sng);

// Loads the whole of the song.
void LoadSongDCFromDir( Directory songPath, std::vector<dotcur::Song*> &VecOut );

#else
#error "GameState.h included twice."
#endif
