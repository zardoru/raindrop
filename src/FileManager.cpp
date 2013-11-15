#include "Global.h"
#include "GameObject.h"
#include "FileManager.h"
#include "NoteLoader.h"
#include "Audio.h"
#include "Directory.h"

#define DirectoryPrefix String("./GameData/")
#define SkinsPrefix String("Skins/")
#define SongsPrefix String("./Songs/")

String FileManager::CurrentSkin = "default";

void loadSong( Directory songPath, std::vector<Song*> &VecOut )
{
	bool FoundDCF = false;
	std::vector<String> Listing;

	songPath.ListDirectory(&Listing, Directory::FS_REG, "dcf");

	// First, search for .dcf files.
	for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		// found a .dcf- load it.
		Song *New = NoteLoader::LoadObjectsFromFile(songPath.path() + "/" + *i, songPath.path());
		if (New)
		{
			New->ChartFilename = *i;
			VecOut.push_back(New);
			FoundDCF = true;
		}
	}

	// If we didn't find any chart, add this song to the list as edit-only.
	if (!FoundDCF)
	{
		Song *NewS = NULL;
		String PotentialBG, PotentialBGRelative;

		Listing.clear();
		songPath.ListDirectory(&Listing, Directory::FS_REG);

		for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{
			if (Utility::GetExtension(*i) == "ogg")
			{
				bool UsesFilename = false;
				NewS = new Song();
				NewS->SongDirectory = songPath.path();
				NewS->SongName = GetOggTitle(songPath.path() + *i);

				if (!NewS->SongName.length())
				{
					UsesFilename = true;
					NewS->SongName = *i;
				}

				NewS->SongRelativePath = *i;
				NewS->SongFilename = (songPath.path() + "/"+ *i);
				NewS->ChartFilename = UsesFilename ? Utility::RemoveExtension(NewS->SongName) + ".dcf" : NewS->SongName + ".dcf";
				VecOut.push_back(NewS);
			}
			else if ( Utility::GetExtension(*i) == "png" || Utility::GetExtension(*i) == "jpg")
			{
				PotentialBG = (songPath.path() + *i);
				PotentialBGRelative = *i;
			}
		}

		if (NewS)
		{
			NewS->BackgroundDir = PotentialBG;
			NewS->BackgroundRelativeDir = PotentialBGRelative;
		}
	}
}

String FileManager::GetDirectoryPrefix()
{
	return DirectoryPrefix;
}

String FileManager::GetSkinPrefix()
{
	// I wonder if a directory transversal is possible. Or useful, for that matter.
	return DirectoryPrefix + SkinsPrefix + CurrentSkin + "/";
}

std::fstream FileManager::OpenFile(String Directory)
{
	return std::fstream ( (DirectoryPrefix + Directory).c_str() );
}

void FileManager::GetSongList(std::vector<Song*> &OutVec)
{
	Directory Dir (SongsPrefix);
	std::vector <String> Listing;
	Dir.ListDirectory(&Listing, Directory::FS_DIR);
	for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{ 
		loadSong(Dir.path() + *i, OutVec);
	}
	
}
