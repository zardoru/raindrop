#ifndef FILEMAN_H_
#define FILEMAN_H_

#include <vector>
#include <fstream>
#include <string>
#include "Song.h"

class FileManager
{
	static std::string CurrentSkin;
public:
	static void GetSongList(std::vector<Song> &OutVec);
	static std::fstream OpenFile(std::string Directory);
	static std::string GetDirectoryPrefix();
	static std::string GetSkinPrefix();
	static void SetSkin(std::string NextSkin);
};

#endif