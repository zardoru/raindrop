#include "GameGlobal.h"
#include "FileManager.h"
#include "Audio.h"
#include "Directory.h"
#include "Configuration.h"

/* Note Loaders */
#include "SongDC.h"
#include "Song7K.h"
#include "NoteLoaderDC.h"
#include "NoteLoader7K.h"

#include <iostream>

#define DirectoryPrefix String("./GameData/")
#define SkinsPrefix String("Skins/")
#define SongsPrefix String("Songs")
#define ScriptsPrefix String("Scripts/")

String FileManager::CurrentSkin = "default";

void loadSong( Directory songPath, std::vector<dotcur::Song*> &VecOut )
{

	bool FoundDCF = false;
	std::vector<String> Listing;

	songPath.ListDirectory(Listing, Directory::FS_REG, "dcf");
	
	// First, search for .dcf files.
	for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		// found a .dcf- load it.
		dotcur::Song *New = NoteLoader::LoadObjectsFromFile(songPath.path() + "/" + *i, songPath.path());
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
		dotcur::Song *NewS = NULL;
		String PotentialBG, PotentialBGRelative;

		Listing.clear();
		songPath.ListDirectory(Listing, Directory::FS_REG);

		for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{
			if (Utility::GetExtension(*i) == "ogg")
			{
				bool UsesFilename = false;
				NewS = new dotcur::Song();
				NewS->SongDirectory = songPath.path();
				NewS->SongName = *i;
				UsesFilename = true;

				NewS->SongFilename = *i;
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
			NewS->BackgroundFilename = PotentialBG;
		}
	}
}

void loadSong7K( Directory songPath, std::vector<VSRG::Song*> &VecOut )
{
	std::vector<String> Listing;

	songPath.ListDirectory(Listing, Directory::FS_REG, "sm");

	// Search .sm files. Same as with dcf.
	for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		// Found it? Load it.
		VSRG::Song *New = NoteLoaderSM::LoadObjectsFromFile(songPath.path() + "/" + *i, songPath.path());
		if (New)
		{
			VecOut.push_back(New);
			return;
		}
	}

	songPath.ListDirectory(Listing, Directory::FS_REG);
	VSRG::Song *New = new VSRG::Song();
	for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		std::wstring Ext = Utility::Widen(Utility::GetExtension(*i));

		// no extension
		if (Ext == L"wav" || Ext == L"ogg" || i->find_first_of('.') == std::wstring::npos) continue;

		std::wstring fn = L"/" + Utility::Widen(*i);
		std::wstring sp = Utility::Widen(songPath.path());
		std::string fn_f = Utility::Narrow(sp + fn);

		if (Ext == L"fmd") // Ftb MetaData
		{
			NoteLoaderFTB::LoadMetadata(fn_f, songPath.path(), New);
		}else if (Ext == L"ftb")
		{
			NoteLoaderFTB::LoadObjectsFromFile(fn_f, songPath.path(), New);
		}else if (Ext == L"osu")
			NoteLoaderOM::LoadObjectsFromFile(fn_f, songPath.path(), New);
		else if (Ext == L"bms" || Ext == L"bme" || Ext == L"bml")
			NoteLoaderBMS::LoadObjectsFromFile(fn_f, songPath.path(), New);
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

void FileManager::GetSongList(std::vector<dotcur::Song*> &OutVec)
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
			wprintf(L"%ls...\n", Utility::Widen(*i).c_str());
			loadSong(Dir.path() + *i, OutVec);
			wprintf(L"ok\n");
		}
	}
}

void FileManager::GetSongList7K(std::vector<VSRG::Song*> &OutVec)
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
			wprintf(L"%ls...\n", Utility::Widen(*i).c_str());
			loadSong7K(Dir.path() + *i, OutVec);
			wprintf(L"ok\n");
		}
	}
}

String FileManager::GetScriptsDirectory()
{
	return DirectoryPrefix + ScriptsPrefix;
}