#include "GameGlobal.h"
#include "Logging.h"

#include "Song.h"
#include "SongDatabase.h"
#include "SongDC.h"
#include "Song7K.h"
#include "SongLoader.h"
#include "NoteLoader7K.h"
#include "NoteLoaderDC.h"

struct loaderVSRGEntry_t {
	const wchar_t* Ext;
	void (*LoadFunc) (GString filename, GString prefix, VSRG::Song* Out);
} LoadersVSRG [] = {
	{L"bms", NoteLoaderBMS::LoadObjectsFromFile},
	{L"bme", NoteLoaderBMS::LoadObjectsFromFile},
	{L"bml", NoteLoaderBMS::LoadObjectsFromFile},
	{L"pms", NoteLoaderBMS::LoadObjectsFromFile},
	{L"sm",  NoteLoaderSM::LoadObjectsFromFile},
	{L"osu", NoteLoaderOM::LoadObjectsFromFile},
	{L"fcf", NoteLoaderFTB::LoadObjectsFromFile}
};

SongLoader::SongLoader(SongDatabase* Database)
{
	DB = Database;
}

void SongLoader::LoadSongDCFromDir( Directory songPath, std::vector<dotcur::Song*> &VecOut )
{

	bool FoundDCF = false;
	std::vector<GString> Listing;

	songPath.ListDirectory(Listing, Directory::FS_REG, "dcf");
	
	// First, search for .dcf files.
	for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		// found a .dcf- load it.
		dotcur::Song *New = NoteLoader::LoadObjectsFromFile(songPath.path() + "/" + *i, songPath.path());
		if (New)
		{
			New->SongDirectory = songPath.path();
			New->ChartFilename = *i;
			VecOut.push_back(New);
			FoundDCF = true;
		}
	}

	// If we didn't find any chart, add this song to the list as edit-only.
	if (!FoundDCF && (Configuration::GetConfigf("OggListing") != 0))
	{
		dotcur::Song *NewS = NULL;
		GString PotentialBG, PotentialBGRelative;

		Listing.clear();
		songPath.ListDirectory(Listing, Directory::FS_REG);

		for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{
			GString Ext = Directory(*i).GetExtension();
			if ( Ext == "ogg")
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
			else if ( Ext == "png" || Ext == "jpg")
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

bool VSRGValidExtension(std::wstring Ext)
{
	for (int i = 0; i < sizeof (LoadersVSRG)/sizeof(loaderVSRGEntry_t); i++)
	{
		if (Ext == LoadersVSRG[i].Ext)
			return true;
	}

	return false;
}

VSRG::Song* LoadSong7KFromFilename(Directory Filename, Directory Prefix, VSRG::Song *Sng)
{
	bool AllocSong = false;
	if (!Sng)
	{
		AllocSong = true;
		Sng = new VSRG::Song();
	}

	std::wstring Ext = Utility::Widen(Filename.GetExtension());

	// no extension
	if (Filename.path().find_first_of('.') == std::wstring::npos || !VSRGValidExtension(Ext))
	{
		if (AllocSong) delete Sng;
		return NULL;
	}

	std::wstring fn;

	if (Prefix.path().length())
		fn = L"/" + Utility::Widen(Filename);
	else
		fn = Utility::Widen(Filename);

	std::wstring sp = Utility::Widen(Prefix);
	GString fn_f = Utility::Narrow(sp + fn);

	for (int i = 0; i < sizeof (LoadersVSRG)/sizeof(loaderVSRGEntry_t); i++)
	{
		if (Ext == LoadersVSRG[i].Ext)
			LoadersVSRG[i].LoadFunc (fn_f, Prefix, Sng);
	}

	return Sng;
}

void SongLoader::LoadSong7KFromDir( Directory songPath, std::vector<VSRG::Song*> &VecOut )
{
	std::vector<GString> Listing;

	songPath.ListDirectory(Listing, Directory::FS_REG);
	VSRG::Song *New = new VSRG::Song();

	New->SongDirectory = songPath.path() + "/";

	/*
		Procedure:
		1.- Check all files if cache needs to be renewed or created.
		2.- If it needs to, load the song again.
		3.- If it loaded the song for either reason, rewrite the difficulty cache.
		4.- If it does not need to be renewed or created, just read the metadata and leave it like that.
	*/

	int ID;
	int SongExists = DB->IsSongDirectory(New->SongDirectory, &ID);
	bool RenewCache = false;
	bool DoReload = false;

	for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		std::wstring Ext = Utility::Widen(Directory(*i).GetExtension());

		// Do a full reload if there's at least one file that needs updating.
		if (VSRGValidExtension(Ext) && (!SongExists || DB->CacheNeedsRenewal(songPath.path() + "/" + *i)))
			DoReload = true;
	}

	if (DoReload)
	{
		for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{
			std::wstring Ext = Utility::Widen(Directory(*i).GetExtension());

			if (VSRGValidExtension(Ext))
			{
				RenewCache = true;
				Log::Printf("%ls (dir)\n", Utility::Widen((*i)).c_str());
				LoadSong7KFromFilename(*i, songPath.path(), New);
			}
		}
	}

	if (!SongExists)
		ID = DB->AddSong(New->SongDirectory, MODE_7K, New);
	else
	{
		if (!RenewCache)
		{
			Log::Printf("%ls (cache)\n", Utility::Widen(New->SongDirectory).c_str());
			DB->GetSongInformation7K (ID, New);
		}
	}

	// Files were modified- we have to renew the difficulty entries as well as the cache itself.
	if (RenewCache)
	{
		for (std::vector<VSRG::Difficulty*>::iterator k = New->Difficulties.begin();
			k != New->Difficulties.end();
			k++)
		{
			DB->AddDifficulty(ID, (*k)->Filename, *k, MODE_7K);
			(*k)->Destroy();
		}
	}

	
	if (New->Difficulties.size())
	{
		VecOut.push_back(New);
	}
	else
	{
		delete New;
	}
}


void SongLoader::GetSongListDC(std::vector<dotcur::Song*> &OutVec, Directory Dir)
{
	std::vector <GString> Listing;

	Dir.ListDirectory(Listing, Directory::FS_DIR);
	for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{ 
		Log::Printf("%ls... ", Utility::Widen(*i).c_str());
		LoadSongDCFromDir(Dir.path() + "/" + *i, OutVec);
		Log::Printf("ok\n");
	}
}

void SongLoader::GetSongList7K(std::vector<VSRG::Song*> &OutVec, Directory Dir)
{
	std::vector <GString> Listing;

	Dir.ListDirectory(Listing, Directory::FS_DIR);
	for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{ 
		Log::Printf("%ls... ", Utility::Widen(*i).c_str());
		LoadSong7KFromDir(Dir.path() + "/" + *i, OutVec);
		Log::Printf("ok\n");
	}
}

VSRG::Song* SongLoader::LoadFromMeta(const VSRG::Song* Meta, VSRG::Difficulty* &CurrentDiff, Directory *FilenameOut)
{
	int SongID;
	VSRG::Song* Out;
	DB->IsSongDirectory(Meta->SongDirectory, &SongID);

	GString fn = DB->GetDifficultyFilename(CurrentDiff->ID);
	*FilenameOut = fn;

	Out = LoadSong7KFromFilename(fn, "", NULL);

	// Copy relevant data
	Out->SongDirectory = Meta->SongDirectory;
	

	/* Find out Difficulty IDs to the recently loaded song's difficulty! */
	bool DifficultyFound = false;
	for (std::vector<VSRG::Difficulty*>::iterator k = Out->Difficulties.begin();
		k != Out->Difficulties.end();
		k++)
	{
		DB->AddDifficulty(SongID, (*k)->Filename, *k, MODE_7K);
		if ((*k)->ID == CurrentDiff->ID) // We've got a match; move onward.
		{
			CurrentDiff = *k;
			DifficultyFound = true;
			break; // We're done here, we've found the difficulty we were trying to load
		}
	}

	if (!DifficultyFound)
	{
		delete Out;
		Out = NULL;
	}

	return Out;
}