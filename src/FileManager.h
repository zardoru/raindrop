#ifndef FILEMAN_H_
#define FILEMAN_H_

#include <vector>
#include <fstream>
#include <string>

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

class FileManager
{
	static String CurrentSkin;

	static SongDatabase* Database;
public:
	static void GetSongListDC(std::vector<dotcur::Song*> &OutVec, Directory Dir);
	static void GetSongList7K(std::vector<VSRG::Song*> &OutVec, Directory Dir);

	static std::fstream& OpenFile(String Directory);
	static String GetDirectoryPrefix();
	static String GetSkinPrefix();
	static String GetScriptsDirectory();
	static void SetSkin(String NextSkin);
	static void Initialize();

	static SongDatabase* GetSongsDatabase();
};

// This loads the meta only from the database.
void LoadSong7KFromDir( Directory songPath, std::vector<VSRG::Song*> &VecOut );

// This loads the whole of the song.
VSRG::Song* LoadSong7KFromFilename(String Filename, String Prefix, VSRG::Song *Sng);

// Loads the whole of the song.
void LoadSongDCFromDir( Directory songPath, std::vector<dotcur::Song*> &VecOut );

#endif
