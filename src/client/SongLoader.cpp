#include <string>

#include "Logging.h"

#include <game/Song.h>
#include <game/NoteLoader7K.h>
#include <game/SingleSongLoad.h>
#include <TextAndFileUtil.h>
#include <cassert>
#include <utility>
#include "SongDatabase.h"
#include "SongLoader.h"
#include "Configuration.h"


using namespace rd;

/* matches SingleSongLoad stuff */
const rd::loaderVSRGEntry_t LoadersVSRG[] = {
        { ".bms",   NoteLoaderBMS::LoadObjectsFromFile },
        { ".bme",   NoteLoaderBMS::LoadObjectsFromFile },
        { ".bml",   NoteLoaderBMS::LoadObjectsFromFile },
        { ".pms",   NoteLoaderBMS::LoadObjectsFromFile },
        { ".sm",    NoteLoaderSM::LoadObjectsFromFile  },
        { ".osu",   NoteLoaderOM::LoadObjectsFromFile  },
        { ".ft2",   NoteLoaderFTB::LoadObjectsFromFile },
        { ".ojn",   NoteLoaderOJN::LoadObjectsFromFile },
        { ".ssc",   NoteLoaderSSC::LoadObjectsFromFile },
        { ".bmson", NoteLoaderBMSON::LoadObjectsFromFile }
};

SongLoader::SongLoader(SongDatabase* Database)
{
    DB = Database;
}

bool VSRGValidExtension(const std::string &s)
{
    for (auto i : LoadersVSRG)
    {
        if (s == i.Ext)
            return true;
    }

    return false;
}

bool ValidBMSExtension(const std::string &s)
{

    for (auto & i : LoadersVSRG)
    {
        if (s == i.Ext &&
			i.LoadFunc == NoteLoaderBMS::LoadObjectsFromFile)
            return true;
    }

    return false;
}



std::shared_ptr<rd::Song> LoadSong7KFromFilename(
        const std::filesystem::path& Filename,
        rd::Song* Sng,
        SongDatabase* DB)
{
    bool AllocSong = false;
    if (!Sng)
    {
        AllocSong = true;
        Sng = new rd::Song();
    }


    // no extension
    if (!Filename.has_extension() || !VSRGValidExtension(Filename.extension().string()))
    {
        if (AllocSong) delete Sng;
        return nullptr;
    }

    Sng->SongDirectory = std::filesystem::absolute(Filename).parent_path();
	auto fn = Filename;

	auto ext = Filename.extension();

    for (auto i : LoadersVSRG)
    {
        if (ext == i.Ext)
        {
            Log::LogPrintf("SongLoader: Load %ls from disk...", fn.wstring().c_str());
            try
            {
                i.LoadFunc(fn, Sng);
                Log::LogPrintf(" ok\n");

				int dindex = 0;
				auto hash = (DB != nullptr) ? DB->GetChartHash(fn) : Utility::GetSha256ForFile(fn);
				for (auto &d : Sng->Difficulties) {
					d->Data->FileHash = hash;
					if (d->Data->IndexInFile == -1) {
						d->Data->IndexInFile = dindex;
						dindex++;
					}
				}

            }
            catch (std::exception &e)
            {
                Log::LogPrintf("SongLoader: Failure loading. Reason: %s \n", e.what());
            }
            break;
        }
    }

    if (AllocSong)
        return std::shared_ptr<rd::Song>(Sng);
    return nullptr;
}



void AddSongToList(std::vector<rd::Song*> &VecOut, rd::Song* Sng)
{
    if (Sng->Difficulties.size())
        VecOut.push_back(Sng);
    else
        delete Sng;
}

CfgVar NoFileGrouping("NoFileGrouping");

void SongLoader::LoadBMS(
        rd::Song * &BMSSong,
        std::filesystem::path File,
        std::map<std::string, rd::Song *> &bmsk,
        std::vector<rd::Song *> & VecOut)
{
	BMSSong->SongDirectory = std::filesystem::absolute(File).parent_path();

	try
	{
		LoadSong7KFromFilename(File, BMSSong, DB);
	}
	catch (std::exception &ex)
	{
		Log::Logf("\nSongLoader::LoadSong7KFromDir(): Exception \"%s\" occurred while loading file \"%ls\"\n",
			ex.what(), File.wstring().c_str());
		Utility::DebugBreak();
	}

	// We found a chart with the same title (and subtitle) already.

	if (!NoFileGrouping) {
		std::string key;
		if (Configuration::GetConfigf("SeparateBySubtitle"))
			key = BMSSong->Title + BMSSong->Subtitle;
		else
			key = BMSSong->Title;

		if (bmsk.find(key) != bmsk.end())
		{
			rd::Song *oldSng = bmsk[key];

			if (!BMSSong->Difficulties.empty()) // BMS charts don't have more than one difficulty anyway.
				oldSng->Difficulties.push_back(BMSSong->Difficulties[0]);

			BMSSong->Difficulties.clear();
			delete BMSSong;
		}
		else // Ah then, don't delete it.
		{
			bmsk[key] = BMSSong;
		}

		BMSSong = new rd::Song;
	}
	else
	{
		DB->AssociateSong(BMSSong);
		AddSongToList(VecOut, BMSSong);
		BMSSong = new rd::Song;
	}
}

std::vector<std::filesystem::path> pathlist(32);

void SongLoader::LoadSong7KFromDir(std::filesystem::path songPath, std::vector<rd::Song*> &VecOut)
{
	if (!std::filesystem::is_directory(songPath))
		return;

    std::filesystem::path SongDirectory = std::filesystem::absolute(songPath);

    /*
        Procedure:
        1.- Check all files if cache needs to be renewed or created.
        2.- If it needs to, load the song again.
        3.- If it loaded the song for either reason, rewrite the difficulty cache.
        4.- If it does not need to be renewed or created, just read the metadata and leave it like that.
    */

    bool RenewCache = false;

    /*
        We want the following:
        All BMS must be packed together.
        All osu!mania charts must be packed together.
        OJNs must be their own chart.
        SMs must be their own chart. (And SSCs have priority if more loaders are supported later)

        Therefore; it's not the song directory which we check, but the difficulties' files.
    */

	pathlist.clear();

    /* First we need to see whether these file need to be renewed.*/
    for (auto &entry: std::filesystem::directory_iterator(SongDirectory))
    {
		const auto &File = entry.path();

        /*
            Some people leave nameless, blank .bms files on their folder.
            This causes the cache to do a full reload, so we check if
            we should just ignore this file.
            It'll be loaded in any case, but not considered for cache.
        */
		if (VSRGValidExtension(File.extension().string()))
		{
			if (DB->CacheNeedsRenewal(File)) {
				Log::LogPrintf("File '%ls' needs renewal.\n", File.wstring().c_str());
				RenewCache = true;
			}

			pathlist.push_back(File);
		}
    }

    // Files were modified- we have to reload the charts.
    if (RenewCache)
    {
		std::map<std::string, rd::Song * > bmsk;

		// These may or not be grouped together.
		auto BMSSong = new rd::Song;

		// Every OJN gets its own Song object.
		auto OJNSong = new rd::Song;

		// osu!mania charts are packed together, with FTB charts.
		auto osuSong = new rd::Song;

		// Stepmania charts get their own song objects too.
		auto smSong = new rd::Song;

        for (auto &entry: pathlist)
        {
			// get extension
			auto Ext = entry.extension().string();

            // We want to group charts with the same title together.
            if (ValidBMSExtension(Ext) || Ext == ".bmson")
            {
				LoadBMS(BMSSong, entry, bmsk, VecOut);
            }

			// .ft2 doesn't need its own entry. 
			if (Ext == ".ojn" || Ext == ".ft2")
            {
                LoadSong7KFromFilename(entry, OJNSong, DB);
                DB->AssociateSong(OJNSong);
                AddSongToList(VecOut, OJNSong);
                OJNSong = new Song;
                OJNSong->SongDirectory = SongDirectory;
            }

			// Add them all to the same song.
            if (Ext == ".osu")
                LoadSong7KFromFilename(entry, osuSong, DB);

			// Same as before.
            if (Ext == ".sm" || Ext == ".ssc")
            {
                LoadSong7KFromFilename(entry, smSong, DB);
                DB->AssociateSong(smSong);
                AddSongToList(VecOut, smSong);
                smSong = new Song;
                smSong->SongDirectory = SongDirectory;
            }
        }

        // AddSongToList() handles the cleanup.
        for (auto & i : bmsk)
        {
            DB->AssociateSong(i.second);
            AddSongToList(VecOut, i.second);
        }

        DB->AssociateSong(OJNSong);
        AddSongToList(VecOut, OJNSong);

        DB->AssociateSong(osuSong);
        AddSongToList(VecOut, osuSong);

        DB->AssociateSong(smSong);
        AddSongToList(VecOut, smSong);
    }
    else // We can reload from cache. We do this on a per-file basis.
    {
        // We need to get the song IDs for every file; it's guaranteed that they exist, in theory.
        int ID = -1;
        std::vector<int> IDList;

		for (auto &File: pathlist)
		{
            int CurrentID = DB->GetSongIDForFile(File);
            if (CurrentID != ID)
            {
                ID = CurrentID;
                IDList.push_back(ID);
            }
        }

        // So now we have our list with song IDs that are present on the current directory.
        // Time to load from cache.
        for (int & i : IDList)
        {
            Song *New = new Song;
            Log::Logf("Song ID %d load from cache...", i);
			try {
				DB->GetSongInformation(i, New);
				New->SongDirectory = SongDirectory;

				// make sure it's a well-formed directory on debug
				assert(std::filesystem::exists(New->SongDirectory));

				AddSongToList(VecOut, New);
				Log::Logf(" ok\n");
			}
			catch (std::exception &e) {
				Log::Logf("Error loading from cache: %s\n", e.what());
			}
        }
    }
}

void SongLoader::GetSongList7K(std::vector<Song*> &OutVec,std::filesystem::path Dir)
{
    std::vector <std::filesystem::path> Listing = Utility::GetFileListing(Dir);

    for (const auto& i: Listing)
    {
		Log::Printf("%s... ", i.c_str());
        LoadSong7KFromDir(i, OutVec);
        Log::Printf("ok\n");
    }
}

std::shared_ptr<Song> SongLoader::LoadFromMeta(const rd::Song* Meta, std::shared_ptr<rd::Difficulty> CurrentDiff, std::filesystem::path &FilenameOut, uint8_t &Index)
{
    std::shared_ptr<Song> Out;

    std::filesystem::path fn = DB->GetDifficultyFilename(CurrentDiff->ID);
    FilenameOut = fn;

	Log::LogPrintf("Loading chart from meta ID %i from %s\n", Meta->ID, fn.string().c_str());
    Out = LoadSong7KFromFilename(fn, nullptr, DB);
    if (!Out) return nullptr;
	
    Index = 0;
    /* Find out Difficulty IDs to the recently loaded song's difficulty! */
    bool DifficultyFound = false;
    for (const auto& k : Out->Difficulties)
    {
        DB->InsertOrUpdateDifficulty(Meta->ID, k.get());
        if (k->ID == CurrentDiff->ID) // We've got a match; move onward.
        {
            CurrentDiff = k;
            DifficultyFound = true;
            break; // We're done here, we've found the difficulty we were trying to load
        }
        Index++;
    }

    if (!DifficultyFound)
        return nullptr;

    return Out;
}