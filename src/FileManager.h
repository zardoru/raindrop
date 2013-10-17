#ifndef FILEMAN_H_
#define FILEMAN_H_

#include <vector>
#include <fstream>
#include <string>
#include "Song.h"

class FileManager
{
	static String CurrentSkin;
public:
	static void GetSongList(std::vector<Song*> &OutVec);
	static std::fstream OpenFile(String Directory);
	static String GetDirectoryPrefix();
	static String GetSkinPrefix();
	static void SetSkin(String NextSkin);
};

#endif