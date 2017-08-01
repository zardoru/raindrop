#include "pch.h"

#include "GameGlobal.h"
#include "Logging.h"

#include "Song.h"
#include "SongDatabase.h"
#include "SongDC.h"
#include "Song7K.h"
#include "SongLoader.h"
#include "NoteLoader7K.h"
#include "NoteLoaderDC.h"

using namespace Game;

struct loaderVSRGEntry_t
{
    const wchar_t* Ext;
    void(*LoadFunc) (std::filesystem::path filename, Game::VSRG::Song* Out);
} LoadersVSRG[] = {
    { L".bms",   NoteLoaderBMS::LoadObjectsFromFile },
    { L".bme",   NoteLoaderBMS::LoadObjectsFromFile },
    { L".bml",   NoteLoaderBMS::LoadObjectsFromFile },
    { L".pms",   NoteLoaderBMS::LoadObjectsFromFile },
    { L".sm",    NoteLoaderSM::LoadObjectsFromFile  },
    { L".osu",   NoteLoaderOM::LoadObjectsFromFile  },
    { L".fcf",   NoteLoaderFTB::LoadObjectsFromFile },
    { L".ojn",   NoteLoaderOJN::LoadObjectsFromFile },
    { L".ssc",   NoteLoaderSSC::LoadObjectsFromFile },
    { L".bmson", NoteLoaderBMSON::LoadObjectsFromFile }
};

SongLoader::SongLoader(SongDatabase* Database)
{
    DB = Database;
}

void SongLoader::LoadSongDCFromDir(std::filesystem::path songPath, std::vector<dotcur::Song*> &VecOut)
{
#ifdef DOTCUR_ENABLED
    bool FoundDCF = false;
	auto Listing = Utility::GetFileListing(songPath);

    // First, search for .dcf files.
    for (auto i: Listing)
    {
		if (i.extension() != ".dcf") continue;

        // found a .dcf- load it.
        dotcur::Song *New = NoteLoaderDC::LoadObjectsFromFile(i, songPath);
        if (New)
        {
            New->SongDirectory = songPath;
            New->ChartFilename = i;
            VecOut.push_back(New);
            FoundDCF = true;
        }
    }

    // If we didn't find any chart, add this song to the list as edit-only.
    if (!FoundDCF && (Configuration::GetConfigf("OggListing") != 0))
    {
        dotcur::Song *NewS = nullptr;
        std::string PotentialBG, PotentialBGRelative;

		Listing = Utility::GetFileListing(songPath);

		for (auto i : Listing)
        {
			std::string Ext = i.extension().string();
            if (Ext == ".ogg")
            {
                bool UsesFilename = false;
                NewS = new dotcur::Song();
                NewS->SongDirectory = songPath;
                NewS->SongName = i.filename().string();
                UsesFilename = true;

                NewS->SongFilename = i.filename().string();
				// todo: dcf

                // NewS->ChartFilename = UsesFilename ? Utility::RemoveExtension(NewS->SongName) + ".dcf" : NewS->SongName + ".dcf";
                VecOut.push_back(NewS);
            }
            else if (Ext == ".png" || Ext == ".jpg")
            {
				PotentialBG = i.string();
                PotentialBGRelative = i.filename().string();
            }
        }

        if (NewS)
        {
            NewS->BackgroundFilename = PotentialBG;
        }
    }
#endif
}

bool VSRGValidExtension(std::wstring Ext)
{
    for (int i = 0; i < sizeof(LoadersVSRG) / sizeof(loaderVSRGEntry_t); i++)
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

std::shared_ptr<Game::VSRG::Song> LoadSong7KFromFilename(const std::filesystem::path& filename, Game::VSRG::Song *Sng)
{
    if (!filename.has_extension())
    {
        return nullptr;
    }

    bool AllocSong = false;
    if (!Sng)
    {
        AllocSong = true;
        Sng = new Game::VSRG::Song();
    }

    Sng->SongDirectory = filename.parent_path();

    for (int i = 0; i < sizeof(LoadersVSRG) / sizeof(loaderVSRGEntry_t); i++)
    {
        if (filename.extension() == LoadersVSRG[i].Ext)
        {
            Log::LogPrintf("Load %s from disk...", Utility::ToU8(filename.wstring()).c_str());
            try
            {
                LoadersVSRG[i].LoadFunc(filename, Sng);
                Log::LogPrintf(" ok\n");
            }
            catch (std::exception &e)
            {
                Log::LogPrintf("Failure loading %s: %s\n", filename.string().c_str(), e.what());
            }
            break;
        }
    }

    if (AllocSong)
        return std::shared_ptr<Game::VSRG::Song>(Sng);
    return nullptr;
}

std::shared_ptr<Game::VSRG::Song> LoadSong7KFromFilename(std::filesystem::path Filename, std::filesystem::path Prefix, Game::VSRG::Song *Sng)
{
	auto prefix = Prefix.string();

    bool AllocSong = false;
    if (!Sng)
    {
        AllocSong = true;
        Sng = new Game::VSRG::Song();
    }

    std::wstring Ext = Utility::Widen(Filename.extension().string());

    // no extension
    if (!Filename.has_extension() || !VSRGValidExtension(Ext))
    {
        if (AllocSong) delete Sng;
        return nullptr;
    }

    Sng->SongDirectory = Prefix;
	auto fn = Prefix / Filename;
	auto fnu8 = Utility::ToU8(fn.wstring());

    for (int i = 0; i < sizeof(LoadersVSRG) / sizeof(loaderVSRGEntry_t); i++)
    {
        if (Ext == LoadersVSRG[i].Ext)
        {
            Log::LogPrintf("SongLoader: Load %s from disk...", fn.string().c_str());
            try
            {
                LoadersVSRG[i].LoadFunc(fn, Sng);
                Log::LogPrintf(" ok\n");
            }
            catch (std::exception &e)
            {
                Log::LogPrintf("SongLoader: Failure loading. Reason: %s \n", e.what());
            }
            break;
        }
    }

    if (AllocSong)
        return std::shared_ptr<Game::VSRG::Song>(Sng);
    return nullptr;
}

void VSRGUpdateDatabaseDifficulties(SongDatabase* DB, Game::VSRG::Song *New)
{
    int ID;

    if (!New->Difficulties.size())
        return;

    // All difficulties have the same song ID, so..
    ID = DB->GetSongIDForFile(New->Difficulties.at(0)->Filename, New);

    // Do the update, with the either new or old difficulty.
    for (auto k = New->Difficulties.begin();
    k != New->Difficulties.end();
        ++k)
    {
        DB->AddDifficulty(ID, (*k)->Filename, k->get(), MODE_VSRG);
        (*k)->Destroy();
    }
}

void PushVSRGSong(std::vector<Game::VSRG::Song*> &VecOut, Game::VSRG::Song* Sng)
{
    if (Sng->Difficulties.size())
        VecOut.push_back(Sng);
    else
        delete Sng;
}

void SongLoader::LoadSong7KFromDir(std::filesystem::path songPath, std::vector<Game::VSRG::Song*> &VecOut)
{
	if (!std::filesystem::is_directory(songPath))
		return;

    std::vector<std::filesystem::path> Listing = Utility::GetFileListing(songPath);
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
        All FCFs to be grouped together

        Therefore; it's not the song directory which we check, but the difficulties' files.
    */

    /* First we need to see whether these file need to be renewed.*/
    for (auto i = Listing.begin(); i != Listing.end(); ++i)
    {
        auto File = *i;
        std::wstring Ext = File.extension().wstring();

        /*
            Some people leave nameless, blank .bms files on their folder.
            This causes the cache to do a full reload, so we check if
            we should just ignore this file.
            It'll be loaded in any case, but not considered for cache.
        */
        std::string Fname = File.filename().string();

		if (VSRGValidExtension(Ext) &&
			Fname.length() &&
			DB->CacheNeedsRenewal(File))
				RenewCache = true;
    }

    // Files were modified- we have to reload the charts.
    if (RenewCache)
    {
		CfgVar NoFileGrouping("NoFileGrouping");
        // First, pack BMS charts together.
        std::map<std::string, Game::VSRG::Song*> bmsk;
        Game::VSRG::Song *BMSSong = new Game::VSRG::Song;

        // Every OJN gets its own Song object.
        Game::VSRG::Song *OJNSong = new Game::VSRG::Song;
        OJNSong->SongDirectory = SongDirectory;

        // osu!mania charts are packed together, with FTB charts.
        Game::VSRG::Song *osuSong = new Game::VSRG::Song;
        osuSong->SongDirectory = SongDirectory;

        // Stepmania charts get their own song objects too.
        Game::VSRG::Song *smSong = new Game::VSRG::Song;
        smSong->SongDirectory = SongDirectory;

        for (auto File : Listing)
        {
            std::wstring Ext = File.extension().wstring();
			File = File.filename();

            // We want to group charts with the same title together.
            if (ValidBMSExtension(Ext) || Ext == L".bmson")
            {
                BMSSong->SongDirectory = SongDirectory;

                try
                {
                    LoadSong7KFromFilename(File, SongDirectory, BMSSong);
                }
                catch (std::exception &ex)
                {
                    Log::Logf("\nSongLoader::LoadSong7KFromDir(): Exception \"%s\" occurred while loading file \"%s\"\n",
                        ex.what(), File.filename().c_str());
                    Utility::DebugBreak();
                }

                // We found a chart with the same title (and subtitle) already.

				if (!NoFileGrouping) {
					std::string key;
					if (Configuration::GetConfigf("SeparateBySubtitle"))
						key = BMSSong->SongName + BMSSong->Subtitle;
					else
						key = BMSSong->SongName;

					if (bmsk.find(key) != bmsk.end())
					{
						Game::VSRG::Song *oldSng = bmsk[key];

						if (BMSSong->Difficulties.size()) // BMS charts don't have more than one difficulty anyway.
							oldSng->Difficulties.push_back(BMSSong->Difficulties[0]);

						BMSSong->Difficulties.clear();
						delete BMSSong;
					}
					else // Ah then, don't delete it.
					{
						bmsk[key] = BMSSong;
					}

					BMSSong = new Game::VSRG::Song;
				} else
				{
					VSRGUpdateDatabaseDifficulties(DB, BMSSong);
					PushVSRGSong(VecOut, BMSSong);
					BMSSong = new Game::VSRG::Song;
				}
            }

            if (Ext == L".ojn")
            {
                LoadSong7KFromFilename(File, SongDirectory, OJNSong);
                VSRGUpdateDatabaseDifficulties(DB, OJNSong);
                PushVSRGSong(VecOut, OJNSong);
                OJNSong = new VSRG::Song;
                OJNSong->SongDirectory = SongDirectory;
            }

            if (Ext == L".osu" || Ext == L".fcf")
                LoadSong7KFromFilename(File, SongDirectory, osuSong);

            if (Ext == L".sm" || Ext == L".ssc")
            {
                LoadSong7KFromFilename(File, SongDirectory, smSong);
                VSRGUpdateDatabaseDifficulties(DB, smSong);
                PushVSRGSong(VecOut, smSong);
                smSong = new VSRG::Song;
                smSong->SongDirectory = SongDirectory;
            }
        }

        // PushVSRGSong() handles the cleanup.
        for (auto i = bmsk.begin();
        i != bmsk.end(); ++i)
        {
            VSRGUpdateDatabaseDifficulties(DB, i->second);
            PushVSRGSong(VecOut, i->second);
        }

        VSRGUpdateDatabaseDifficulties(DB, OJNSong);
        PushVSRGSong(VecOut, OJNSong);

        VSRGUpdateDatabaseDifficulties(DB, osuSong);
        PushVSRGSong(VecOut, osuSong);

        VSRGUpdateDatabaseDifficulties(DB, smSong);
        PushVSRGSong(VecOut, smSong);
    }
    else // We can reload from cache. We do this on a per-file basis.
    {
        // We need to get the song IDs for every file; it's guaranteed that they exist, in theory.
        int ID = -1;
        std::vector<int> IDList;

        for (auto File : Listing)
        {
            std::wstring Ext = File.extension().wstring();
            if (VSRGValidExtension(Ext))
            {
                int CurrentID = DB->GetSongIDForFile(File, nullptr);
                if (CurrentID != ID)
                {
                    ID = CurrentID;
                    IDList.push_back(ID);
                }
            }
        }

        // So now we have our list with song IDs that are present on the current directory.
        // Time to load from cache.
        for (auto i = IDList.begin();
        i != IDList.end();
            ++i)
        {
            VSRG::Song *New = new VSRG::Song;
            Log::Logf("Song ID %d load from cache...", *i);
			try {
				DB->GetSongInformation(*i, New);
				New->SongDirectory = SongDirectory;

				// make sure it's a well-formed directory on debug
				assert(std::filesystem::exists(New->SongDirectory));

				PushVSRGSong(VecOut, New);
				Log::Logf(" ok\n");
			}
			catch (std::exception &e) {
				Log::Logf("Error loading from cache: %s\n", e.what());
			}
        }
    }
}

void SongLoader::GetSongListDC(std::vector<dotcur::Song*> &OutVec, std::filesystem::path Dir)
{
    std::vector <std::filesystem::path> Listing = Utility::GetFileListing(Dir);

    for (auto i: Listing)
    {
        Log::Printf("%s... ", i.c_str());
        LoadSongDCFromDir(i, OutVec);
        Log::Printf("ok\n");
    }
}

void SongLoader::GetSongList7K(std::vector<VSRG::Song*> &OutVec,std::filesystem::path Dir)
{
    std::vector <std::filesystem::path> Listing = Utility::GetFileListing(Dir);

    for (auto i: Listing)
    {
		Log::Printf("%s... ", i.c_str());
        LoadSong7KFromDir(i, OutVec);
        Log::Printf("ok\n");
    }
}

std::shared_ptr<VSRG::Song> SongLoader::LoadFromMeta(const Game::VSRG::Song* Meta, std::shared_ptr<Game::VSRG::Difficulty> CurrentDiff, std::filesystem::path &FilenameOut, uint8_t &Index)
{
    std::shared_ptr<VSRG::Song> Out;

    std::filesystem::path fn = DB->GetDifficultyFilename(CurrentDiff->ID);
    FilenameOut = fn;

    Out = LoadSong7KFromFilename(fn, "", nullptr);
    if (!Out) return nullptr;

    // Copy relevant data
    Out->SongDirectory = Meta->SongDirectory;

    Index = 0;
    /* Find out Difficulty IDs to the recently loaded song's difficulty! */
    bool DifficultyFound = false;
    for (auto k : Out->Difficulties)
    {
        DB->AddDifficulty(Meta->ID, k->Filename, k.get(), MODE_VSRG);
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