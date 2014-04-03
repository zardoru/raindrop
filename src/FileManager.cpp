#include "Global.h"
#include "GameObject.h"
#include "FileManager.h"
#include "Audio.h"
#include "Directory.h"
#include "Configuration.h"

/* Note Loaders */
#include "NoteLoader.h"
#include "NoteLoader7K.h"

#include <iostream>

#define DirectoryPrefix String("./GameData/")
#define SkinsPrefix String("Skins/")
#define SongsPrefix String("Songs/")
#define ScriptsPrefix String("Scripts/")

String FileManager::CurrentSkin = "default";

void loadSong( Directory songPath, std::vector<SongDC*> &VecOut )
{

	bool FoundDCF = false;
	std::vector<String> Listing;

	songPath.ListDirectory(Listing, Directory::FS_REG, "dcf");
	
	// First, search for .dcf files.
	for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		// found a .dcf- load it.
		SongDC *New = NoteLoader::LoadObjectsFromFile(songPath.path() + "/" + *i, songPath.path());
		if (New)
		{
			New->ChartFilename = *i;
			VecOut.push_back(New);
			FoundDCF = true;
		}
	}

	// If we didn't find any chart, add this song to the list as edit-only.
	if (!FoundDCF && (Configuration::GetConfigf("OggListing") != 0))
	{
		SongDC *NewS = NULL;
		String PotentialBG, PotentialBGRelative;

		Listing.clear();
		songPath.ListDirectory(Listing, Directory::FS_REG);

		for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{
			if (Utility::GetExtension(*i) == "ogg")
			{
				bool UsesFilename = false;
				NewS = new SongDC();
				NewS->SongDirectory = songPath.path();
				NewS->SongName = *i;
				UsesFilename = true;

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

void loadSong7K( Directory songPath, std::vector<Song7K*> &VecOut )
{
	std::vector<String> Listing;

	songPath.ListDirectory(Listing, Directory::FS_REG, "sm");

	// Search .sm files. Same as with dcf.
	for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		// Found it? Load it.
		Song7K *New = NoteLoaderSM::LoadObjectsFromFile(songPath.path() + "/" + *i, songPath.path());
		if (New)
		{
			New->ChartFilename = *i;
			VecOut.push_back(New);
			return;
		}
	}

	songPath.ListDirectory(Listing, Directory::FS_REG);
	Song7K *New = new Song7K();
	for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		String Ext = Utility::GetExtension(*i);
		if (Ext == "fmd") // Ftb MetaData
		{
			NoteLoaderFTB::LoadMetadata(songPath.path() + "/" + *i, songPath.path(), New);
		}else if (Ext == "ftb")
		{
			NoteLoaderFTB::LoadObjectsFromFile(songPath.path() + "/" + *i, songPath.path(), New);
		}else if (Ext == "osu")
			NoteLoaderOM::LoadObjectsFromFile(songPath.path() + "/" + *i, songPath.path(), New);
		else if (Ext == "bms" || Ext == "bme")
			NoteLoaderBMS::LoadObjectsFromFile(songPath.path() + "/" + *i, songPath.path(), New);
	}

	if (New->Difficulties.size())
		VecOut.push_back(New);
	else
		delete New;
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

void FileManager::SetSkin(String Skin)
{
	CurrentSkin = Skin;
}

std::fstream& FileManager::OpenFile(String Directory)
{
	std::fstream* f = new std::fstream( (DirectoryPrefix + Directory).c_str() );
	return *f;
}

void FileManager::GetSongList(std::vector<SongDC*> &OutVec)
{
	std::vector <String> SongDirectories;
	SongDirectories.push_back(SongsPrefix);

	Configuration::GetConfigListS ("SongDirectories", SongDirectories);

	for (std::vector<String>::iterator i = SongDirectories.begin(); i != SongDirectories.end(); i++)
	{
		Directory Dir (*i + "/");
		std::vector <String> Listing;

		Dir.ListDirectory(Listing, Directory::FS_DIR);
		for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{ 
			std::cout << *i << "... ";
			loadSong(Dir.path() + *i, OutVec);
			std::cout << "ok\n";
		}
	}
}

void FileManager::GetSongList7K(std::vector<Song7K*> &OutVec)
{
	std::vector <String> SongDirectories;
	SongDirectories.push_back(SongsPrefix);

	Configuration::GetConfigListS ("SongDirectories", SongDirectories);

	for (std::vector<String>::iterator i = SongDirectories.begin(); i != SongDirectories.end(); i++)
	{
		Directory Dir (*i + "/");
		std::vector <String> Listing;

		Dir.ListDirectory(Listing, Directory::FS_DIR);
		for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{ 
			std::cout << *i << "... ";
			loadSong7K(Dir.path() + *i, OutVec);
			std::cout << "ok\n";
		}
	}
}

String FileManager::GetScriptsDirectory()
{
	return DirectoryPrefix + ScriptsPrefix;
}