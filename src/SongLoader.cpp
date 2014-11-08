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
	{ L"bms",  NoteLoaderBMS::LoadObjectsFromFile },
	{ L"bme",  NoteLoaderBMS::LoadObjectsFromFile },
	{ L"bml",  NoteLoaderBMS::LoadObjectsFromFile },
	{ L"pms",  NoteLoaderBMS::LoadObjectsFromFile },
	{ L"sm",   NoteLoaderSM::LoadObjectsFromFile },
	{ L"osu",  NoteLoaderOM::LoadObjectsFromFile },
	{ L"fcf",  NoteLoaderFTB::LoadObjectsFromFile },
	{ L"ojn",  NoteLoaderOJN::LoadObjectsFromFile }
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
			if ( Ext == "ogg" )
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

bool ValidBMSExtension(std::wstring Ext)
{
	for (int i = 0; i < sizeof(LoadersVSRG) / sizeof(loaderVSRGEntry_t); i++)
	{
		if (Ext == LoadersVSRG[i].Ext && LoadersVSRG[i].LoadFunc == NoteLoaderBMS::LoadObjectsFromFile)
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

	fn = Utility::Widen(Filename);

	std::wstring sp = Utility::Widen(Prefix);
	GString fn_f = Utility::Narrow(sp + fn);

	Sng->SongDirectory = Prefix;

	for (int i = 0; i < sizeof (LoadersVSRG)/sizeof(loaderVSRGEntry_t); i++)
	{
		if (Ext == LoadersVSRG[i].Ext)
		{
			Log::Printf("Load %s from disk...", Filename.c_path());
			LoadersVSRG[i].LoadFunc (fn_f, Prefix, Sng);
			Log::Printf(" ok\n");
		}
	}

	return Sng;
}

void VSRGUpdateDatabaseDifficulties(SongDatabase* DB, VSRG::Song *New)
{
	int ID;

	if (!New->Difficulties.size())
		return;

	// All difficulties have the same song ID, so..
	ID = DB->GetSongIDForFile(New->Difficulties.at(0)->Filename, New);

	// Do the update, with the either new or old difficulty.
	for (std::vector<VSRG::Difficulty*>::iterator k = New->Difficulties.begin();
		k != New->Difficulties.end();
		k++)
	{
		DB->AddDifficulty(ID, (*k)->Filename, *k, MODE_VSRG);
		(*k)->Destroy();
	}
}

void PushVSRGSong(std::vector<VSRG::Song*> &VecOut, VSRG::Song* Sng)
{
	if (Sng->Difficulties.size())
		VecOut.push_back(Sng);
	else
		delete Sng;
}

void SongLoader::LoadSong7KFromDir( Directory songPath, std::vector<VSRG::Song*> &VecOut )
{
	std::vector<GString> Listing;

	songPath.ListDirectory(Listing, Directory::FS_REG);

	Directory SongDirectory = songPath.path() + "/";

	/*
		Procedure:
		1.- Check all files if cache needs to be renewed or created.
		2.- If it needs to, load the song again.
		3.- If it loaded the song for either reason, rewrite the difficulty cache.
		4.- If it does not need to be renewed or created, just read the metadata and leave it like that.
	*/

	bool RenewCache = false;
	bool DoReload = false;

	/*
		We want the following:
		All BMS must be packed together.
		All osu!mania charts must be packed together.
		OJNs must be their own chart.
		SMs must be their own chart. (And SSCs have priority if more loaders are supported later)
		All FCFs to be grouped together

		Therefore; it's not the song directory which we check, but the difficulties' files.
	*/

	/* First we need to see whether these file need to be renewed.*/
	for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
	{
		std::wstring Ext = Utility::Widen(Directory(*i).GetExtension());

		if ( VSRGValidExtension(Ext) && DB->CacheNeedsRenewal(SongDirectory / *i) )
			RenewCache = true;
	}

	// Files were modified- we have to reload the charts.
	if (RenewCache)
	{
		// First, pack BMS charts together.
		std::map<GString, VSRG::Song*> bmsk;
		VSRG::Song *BMSSong = new VSRG::Song;

		// We want to group charts with the same title together.
		for (auto i = Listing.begin(); i != Listing.end(); i++)
		{
			std::wstring Ext = Utility::Widen(Directory(*i).GetExtension());
			
			if (ValidBMSExtension(Ext))
			{
				BMSSong->SongDirectory = SongDirectory;

				LoadSong7KFromFilename(*i, SongDirectory, BMSSong);

				if (bmsk.find(BMSSong->SongName) != bmsk.end()) // We found a chart with the same title already.
				{
					VSRG::Song *oldSng = bmsk[BMSSong->SongName];

					if (BMSSong->Difficulties.size()) // BMS charts don't have more than one difficulty anyway.
						oldSng->Difficulties.push_back(BMSSong->Difficulties[0]);

					BMSSong->Difficulties.clear();
					delete BMSSong;
				}
				else // Ah then, don't delete it.
				{
					bmsk[BMSSong->SongName] = BMSSong;
				}
					
				BMSSong = new VSRG::Song;
			}
		}

		for (auto i = bmsk.begin();
			i != bmsk.end(); i++)
		{
			VSRGUpdateDatabaseDifficulties(DB, i->second);
			PushVSRGSong(VecOut, i->second);
		}

		// Every OJN gets its own Song object.
		VSRG::Song *OJNSong = new VSRG::Song;
		OJNSong->SongDirectory = SongDirectory;
		for (auto i = Listing.begin(); i != Listing.end(); i++)
		{
			std::wstring Ext = Utility::Widen(Directory(*i).GetExtension());

			if (Ext == L"ojn")
			{
				LoadSong7KFromFilename(*i, SongDirectory, OJNSong);
				VSRGUpdateDatabaseDifficulties(DB, OJNSong);
				PushVSRGSong(VecOut, OJNSong);
				OJNSong = new VSRG::Song;
				OJNSong->SongDirectory = SongDirectory;
			}
		}

		VSRGUpdateDatabaseDifficulties(DB, OJNSong);
		PushVSRGSong(VecOut, OJNSong);

		
		// osu!mania charts are packed together, with FTB charts.
		VSRG::Song *osuSong = new VSRG::Song;
		osuSong->SongDirectory = SongDirectory;
		for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{
			std::wstring Ext = Utility::Widen(Directory(*i).GetExtension());

			if (Ext == L"osu" || Ext == L"fcf")
				LoadSong7KFromFilename(*i, SongDirectory, osuSong);
		}

		VSRGUpdateDatabaseDifficulties(DB, osuSong);
		PushVSRGSong(VecOut, osuSong);

		// Stepmania charts get their own song objects too.
		VSRG::Song *smSong = new VSRG::Song;
		smSong->SongDirectory = SongDirectory;
		for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{
			std::wstring Ext = Utility::Widen(Directory(*i).GetExtension());

			if (Ext == L"sm")
			{
				LoadSong7KFromFilename(*i, SongDirectory, smSong);
				VSRGUpdateDatabaseDifficulties(DB, smSong);
				PushVSRGSong(VecOut, smSong);
				smSong = new VSRG::Song;
				smSong->SongDirectory = SongDirectory;
			}
		}

		VSRGUpdateDatabaseDifficulties(DB, smSong);
		PushVSRGSong(VecOut, smSong);
	}
	else // We can reload from cache. We do this on a per-file basis.
	{
		// We need to get the song IDs for every file; it's guaranteed that they exist, in theory.
		int ID = -1;
		std::vector<int> IDList;

		for (std::vector<GString>::iterator i = Listing.begin(); i != Listing.end(); i++)
		{
			std::wstring Ext = Utility::Widen(Directory(*i).GetExtension());
			if (VSRGValidExtension(Ext))
			{
				assert(!DB->CacheNeedsRenewal(SongDirectory / *i));
				int CurrentID = DB->GetSongIDForFile(SongDirectory / *i, NULL);
				if (CurrentID != ID)
				{
					ID = CurrentID;
					IDList.push_back(ID);
				}
			}
		}

		// So now we have our list with song IDs that are present on the current directory.
		// Time to load from cache.
		for (std::vector<int>::iterator i = IDList.begin();
			i != IDList.end();
			i++)
		{
			VSRG::Song *New = new VSRG::Song;
			Log::Printf("Song ID %d load from cache...", *i);
			DB->GetSongInformation7K(*i, New);
			New->SongDirectory = SongDirectory;

			PushVSRGSong(VecOut, New);
			Log::Printf(" ok\n");
		}
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
	VSRG::Song* Out;

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
		DB->AddDifficulty(Meta->ID, (*k)->Filename, *k, MODE_VSRG);
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