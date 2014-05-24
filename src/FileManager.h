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
	class Song;
}

class FileManager
{
	static String CurrentSkin;
public:
	static void GetSongList(std::vector<dotcur::Song*> &OutVec);
	static void GetSongList7K(std::vector<VSRG::Song*> &OutVec);
	static std::fstream& OpenFile(String Directory);
	static String GetDirectoryPrefix();
	static String GetSkinPrefix();
	static String GetScriptsDirectory();
	static void SetSkin(String NextSkin);
};

#endif
