#include "Global.h"
#include "GameObject.h"
#include "FileManager.h"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "NoteLoader.h"

#define DirectoryPrefix std::string("./GameData/")
#define SkinsPrefix std::string("Skins/")
#define SongsPrefix L"./Songs/"

std::string FileManager::CurrentSkin = "default";

void loadSong( boost::filesystem::path songPath, std::vector<Song*> &VecOut )
{
	bool FoundDCF = false;

	using namespace boost::filesystem;
	directory_iterator end_iter;
	
	std::string Path = songPath.string();

	// First, search for .dcf files.
	for (directory_iterator it (songPath); it != end_iter ; it++)
	{
		if (is_regular_file(it->path()))
		{
			if ( extension(it->path()) == (".dcf") )
			{
				// found a .dcf- load it.
				VecOut.push_back(NoteLoader::LoadObjectsFromFile(it->path().string(), songPath.string()));
				FoundDCF = true;
			}
		}
	}

	// If we didn't find any chart, add this song to the list as edit-only.
	if (!FoundDCF)
	{
		for (directory_iterator it (songPath); it != end_iter ; it++)
		{
			if (is_regular_file(it->path()))
			{
				if ( extension(it->path()) == (".ogg") ) // Found an OGG file?
				{
					Song *NewS;
					NewS = new Song();
					NewS->SongDirectory = Path;
					NewS->SongName = it->path().filename().string();
					NewS->SongFilename = it->path().string();
					VecOut.push_back(NewS);
				}
			}
		}
	}
	
}

std::string FileManager::GetDirectoryPrefix()
{
	return DirectoryPrefix;
}

std::string FileManager::GetSkinPrefix()
{
	// I wonder if a directory transversal is possible. Or useful, for that matter.
	return DirectoryPrefix + SkinsPrefix + CurrentSkin + "/";
}

std::fstream FileManager::OpenFile(std::string Directory)
{
	return std::fstream ( (DirectoryPrefix + Directory).c_str() );
}

void FileManager::GetSongList(std::vector<Song*> &OutVec)
{
	using namespace boost::filesystem;
	path songsPath (SongsPrefix);
	directory_iterator it(songsPath), eod;

	BOOST_FOREACH(path const &p, std::make_pair(it, eod))   
	{ 
		if(is_directory(p))
		{
			loadSong(p, OutVec);
		} 
	}
	
}
