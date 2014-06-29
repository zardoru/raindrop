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
	static void GetSongList(std::vector<dotcur::Song*> &OutVec);
	static void GetSongList7K(std::vector<VSRG::Song*> &OutVec);

	static String GetCacheFilename(String Fn, String Nm);

	static std::fstream& OpenFile(String Directory);
	static String GetDirectoryPrefix();
	static String GetSkinPrefix();
	static String GetScriptsDirectory();
	static String GetCacheDirectory();
	static void SetSkin(String NextSkin);
	static void Initialize();
};

VSRG::Song* LoadSong7KFromFilename(String Filename, String Prefix, VSRG::Song *Sng);
String GenHash(String Str, int Sd = 0);

#endif
