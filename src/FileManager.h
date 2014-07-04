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

class FileManager
{
	static String CurrentSkin;
public:
	static void GetSongListDC(std::vector<dotcur::Song*> &OutVec, Directory Dir);
	static void GetSongList7K(std::vector<VSRG::Song*> &OutVec, Directory Dir);

	static std::fstream& OpenFile(String Directory);
	static String GetDirectoryPrefix();
	static String GetSkinPrefix();
	static String GetScriptsDirectory();
	static String GetCacheDirectory();
	static void SetSkin(String NextSkin);
	static void Initialize();
};

void LoadSong7KFromDir( Directory songPath, std::vector<VSRG::Song*> &VecOut );
VSRG::Song* LoadSong7KFromFilename(String Filename, String Prefix, VSRG::Song *Sng);
void LoadSongDCFromDir( Directory songPath, std::vector<dotcur::Song*> &VecOut );

#endif
