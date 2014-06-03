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

#define DirectoryPrefix String("GameData/")
#define SkinsPrefix String("Skins/")
#define SongsPrefix String("Songs")
#define ScriptsPrefix String("Scripts/")
#define CachePrefix String("Cache/")

String FileManager::CurrentSkin = "default";

void FileManager::Initialize()
{
	Utility::CheckDir(GetCacheDirectory().c_str());
}

String FileManager::GetCacheDirectory()
{
	return CachePrefix;
}

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


String GenHash(String Dir)
{
	size_t Sum = 0;
	for (size_t i = 0; i < Dir.length(); i++)
	{
		Sum += Dir[i];
	}

	std::stringstream ss;
	ss << Sum;
	return ss.str();
}

void WriteMetaCache(VSRG::Song *Sng, String filename)
{
	std::fstream out(filename.c_str(), std::ios::out);
	out << Sng->SongAuthor << "\n";
	out << Sng->SongName << "\n";
	out << Sng->BPMType << "\n";
	out << Sng->BackgroundFilename << "\n";

	for (std::vector<VSRG::Difficulty*>::iterator k = Sng->Difficulties.begin();
			k != Sng->Difficulties.end();
			k++)
	{
		out << "$\n";

		out << (*k)->Name << std::endl;
		out << (int)(*k)->Channels << " "
			<< (*k)->Duration << " "
			<< (*k)->IsVirtual << " "
			<< (*k)->Offset << " "
			<< (*k)->TotalHolds << " "
			<< (*k)->TotalNotes << " "
			<< (*k)->TotalObjects << " "
			<< (*k)->TotalScoringObjects
			<< std::endl;

	}
}

void LoadMetaCache(VSRG::Song *Sng, String filename)
{
	std::ifstream in(filename.c_str());
	String tmp;

	if (!in.is_open())
		return;

	std::getline(in, Sng->SongAuthor);
	std::getline(in, Sng->SongName);
	std::getline(in, tmp);
	Sng->BPMType = (VSRG::Song::EBt)atoi(tmp.c_str());
	std::getline(in, Sng->BackgroundFilename);

	while (std::getline(in, tmp))
	{
		VSRG::Difficulty *Diff = new VSRG::Difficulty();
		std::getline(in, Diff->Name);
		in >> Diff->Channels
			>> Diff->Duration
			>> Diff->IsVirtual
			>> Diff->Offset
			>> Diff->TotalHolds
			>> Diff->TotalNotes
			>> Diff->TotalObjects
			>> Diff->TotalScoringObjects;
	}
}

bool VSRGValidExtension(std::wstring Ext)
{
	if (Ext == L"fmd") // Ftb MetaData
		return true;
	else if (Ext == L"ftb")
		return true;
	else if (Ext == L"osu")
		return true;
	else if (Ext == L"bms" || Ext == L"bme" || Ext == L"bml")
		return true;
	else if (Ext == L"sm")
		return true;
	else return false;

}

VSRG::Song* LoadSong7KFromFilename(String Filename, String Prefix, VSRG::Song *Sng)
{
	bool AllocSong = false;
	if (!Sng)
	{
		AllocSong = true;
		Sng = new VSRG::Song();
	}

	std::wstring Ext = Utility::Widen(Utility::GetExtension(Filename));

	// no extension
	if (!VSRGValidExtension(Ext) || Filename.find_first_of('.') == std::wstring::npos)
	{
		if (AllocSong) delete Sng;
		return NULL;
	}

	std::wstring fn = L"/" + Utility::Widen(Filename);
	std::wstring sp = Utility::Widen(Prefix);
	std::string fn_f = Utility::Narrow(sp + fn);

	if (Ext == L"fmd") // Ftb MetaData
		NoteLoaderFTB::LoadMetadata(fn_f, Prefix, Sng);
	else if (Ext == L"ftb")
		NoteLoaderFTB::LoadObjectsFromFile(fn_f, Prefix, Sng);
	else if (Ext == L"osu")
		NoteLoaderOM::LoadObjectsFromFile(fn_f, Prefix, Sng);
	else if (Ext == L"bms" || Ext == L"bme" || Ext == L"bml")
		NoteLoaderBMS::LoadObjectsFromFile(fn_f, Prefix, Sng);
	else if (Ext == L"sm")
		NoteLoaderSM::LoadObjectsFromFile(Prefix + "/" + Filename, Prefix, Sng);

	return Sng;
}

void loadSong7K( Directory songPath, std::vector<VSRG::Song*> &VecOut )
{
	std::vector<String> Listing;

	songPath.ListDirectory(Listing, Directory::FS_REG);
	VSRG::Song *New = new VSRG::Song();

	String Hash = GenHash(songPath.path());
	String FilenameCache = "cache/" + Hash;

	// Find if there exists a cache file for this directory
	for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		std::wstring Ext = Utility::Widen(Utility::GetExtension(*i));
		if (VSRGValidExtension(Ext))
		{
			if (Utility::FileExists(FilenameCache))
			{
				LoadMetaCache(New, FilenameCache);
				break; // Found it, move on.
			}
		}
	}

	// If it doesn't, generate cache files.
	if (!Utility::FileExists(FilenameCache))
	{
		for (std::vector<String>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{
			std::wstring Ext = Utility::Widen(Utility::GetExtension(*i));
			if (VSRGValidExtension(Ext))
				LoadSong7KFromFilename(*i, songPath.path(), New);
		}

		WriteMetaCache(New, FilenameCache);

		for (std::vector<VSRG::Difficulty*>::iterator k = New->Difficulties.begin();
			k != New->Difficulties.end();
			k++)
		{
			(*k)->SaveCache(FilenameCache + (*k)->Name);
			(*k)->Destroy();
		}
	}

	New->FilenameCache = FilenameCache;


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