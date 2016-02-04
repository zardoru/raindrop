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

struct loaderVSRGEntry_t
{
    const wchar_t* Ext;
    void(*LoadFunc) (std::string filename, std::string prefix, VSRG::Song* Out);
} LoadersVSRG[] = {
    { L"bms",   NoteLoaderBMS::LoadObjectsFromFile },
    { L"bme",   NoteLoaderBMS::LoadObjectsFromFile },
    { L"bml",   NoteLoaderBMS::LoadObjectsFromFile },
    //{ L".bml",  NoteLoaderBMS::LoadObjectsFromFile },
    { L"pms",   NoteLoaderBMS::LoadObjectsFromFile },
    { L"sm",    NoteLoaderSM::LoadObjectsFromFile  },
    { L"osu",   NoteLoaderOM::LoadObjectsFromFile  },
    { L"fcf",   NoteLoaderFTB::LoadObjectsFromFile },
    { L"ojn",   NoteLoaderOJN::LoadObjectsFromFile },
    { L"ssc",   NoteLoaderSSC::LoadObjectsFromFile },
    { L"bmson", NoteLoaderBMSON::LoadObjectsFromFile }
};

struct loaderVSRGEntry2_t
{
    const wchar_t* Ext;
    void(*LoadFunc) (const std::filesystem::path&, VSRG::Song* Out);
} LoadersVSRG2[] = {
    { L"bml",   NoteLoaderBMS::LoadObjectsFromFile },
    { L".bml",  NoteLoaderBMS::LoadObjectsFromFile }
};

SongLoader::SongLoader(SongDatabase* Database)
{
    DB = Database;
}

void SongLoader::LoadSongDCFromDir(Directory songPath, std::vector<dotcur::Song*> &VecOut)
{
    bool FoundDCF = false;
    std::vector<std::string> Listing;

    songPath.ListDirectory(Listing, Directory::FS_REG, "dcf");

    // First, search for .dcf files.
    for (std::vector<std::string>::iterator i = Listing.begin(); i != Listing.end(); i++)
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
        dotcur::Song *NewS = nullptr;
        std::string PotentialBG, PotentialBGRelative;

        Listing.clear();
        songPath.ListDirectory(Listing, Directory::FS_REG);

        for (std::vector<std::string>::iterator i = Listing.begin(); i != Listing.end(); i++)
        {
            std::string Ext = Directory(*i).GetExtension();
            if (Ext == "ogg")
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
            else if (Ext == "png" || Ext == "jpg")
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

std::shared_ptr<VSRG::Song> LoadSong7KFromFilename(const std::filesystem::path& filename, VSRG::Song *Sng)
{
    if (!filename.has_extension())
    {
        return nullptr;
    }

    bool AllocSong = false;
    if (!Sng)
    {
        AllocSong = true;
        Sng = new VSRG::Song();
    }

    auto pref = filename.parent_path().string().c_str();
    Sng->SongDirectory = Directory(pref);

    for (int i = 0; i < sizeof(LoadersVSRG2) / sizeof(loaderVSRGEntry_t); i++)
    {
        if (filename.extension() == LoadersVSRG2[i].Ext)
        {
            Log::Logf("Load %s from disk...", filename.string().c_str());
            try
            {
                LoadersVSRG2[i].LoadFunc(filename, Sng);
                Log::Logf(" ok\n");
            }
            catch (std::exception &e)
            {
                Log::LogPrintf("Failure loading %s: %s\n", filename.string().c_str(), e.what());
            }
            break;
        }
    }

    if (AllocSong)
        return std::shared_ptr<VSRG::Song>(Sng);
    return nullptr;
}

std::shared_ptr<VSRG::Song> LoadSong7KFromFilename(Directory Filename, Directory Prefix, VSRG::Song *Sng)
{
    auto filename = std::filesystem::path{ Filename.path() };
    auto directory = std::filesystem::path{ Prefix.path() };

    //return LoadSong7KFromFilename(directory / filename, Sng);

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
        return nullptr;
    }

    std::wstring fn;
    std::wstring sp;
    std::string pref = Prefix.path();

    // Append a / at the end.
    int q = pref.length() - 1;
    if (q > 0)
    {
        char c = pref[q];
        if (c != '/' && c != '\\')
            pref += "/";
    }

    // If the directory is just a dot, add an extra / at the end
    if (q == 0 && pref[q] == '.')
        pref += "/";

    fn = Utility::Widen(Filename);
    sp = Utility::Widen(pref);

    std::string fn_f = Utility::Narrow(sp + fn);

    Sng->SongDirectory = Prefix;

    for (int i = 0; i < sizeof(LoadersVSRG) / sizeof(loaderVSRGEntry_t); i++)
    {
        if (Ext == LoadersVSRG[i].Ext)
        {
            Log::LogPrintf("Load %s from disk...", fn_f.c_str());
            try
            {
                LoadersVSRG[i].LoadFunc(fn_f, Prefix, Sng);
                Log::Logf(" ok\n");
            }
            catch (std::exception &e)
            {
                Log::LogPrintf("Failure loading %s: %s\n", fn_f.c_str(), e.what());
            }
            break;
        }
    }

    if (AllocSong)
        return std::shared_ptr<VSRG::Song>(Sng);
    return nullptr;
}

void VSRGUpdateDatabaseDifficulties(SongDatabase* DB, VSRG::Song *New)
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

void PushVSRGSong(std::vector<VSRG::Song*> &VecOut, VSRG::Song* Sng)
{
    if (Sng->Difficulties.size())
        VecOut.push_back(Sng);
    else
        delete Sng;
}

void SongLoader::LoadSong7KFromDir(Directory songPath, std::vector<VSRG::Song*> &VecOut)
{
    std::vector<std::string> Listing;

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
        Directory File = *i;
        File.Normalize();

        std::wstring Ext = Utility::Widen(File.GetExtension());

        /*
            Some people leave nameless, blank .bms files on their folder.
            This causes the cache to do a full reload, so we check if
            we should just ignore this file.
            It'll be loaded in any case, but not considered for cache.
        */
        std::string Fname = Utility::RemoveExtension(File.Filename());

        if (VSRGValidExtension(Ext) &&
            Fname.length() &&
            DB->CacheNeedsRenewal(SongDirectory / File.path()))
            RenewCache = true;
    }

    // Files were modified- we have to reload the charts.
    RenewCache = true; // TESTING
    if (RenewCache)
    {
        // First, pack BMS charts together.
        std::map<std::string, VSRG::Song*> bmsk;
        VSRG::Song *BMSSong = new VSRG::Song;

        // Every OJN gets its own Song object.
        VSRG::Song *OJNSong = new VSRG::Song;
        OJNSong->SongDirectory = SongDirectory;

        // osu!mania charts are packed together, with FTB charts.
        VSRG::Song *osuSong = new VSRG::Song;
        osuSong->SongDirectory = SongDirectory;

        // Stepmania charts get their own song objects too.
        VSRG::Song *smSong = new VSRG::Song;
        smSong->SongDirectory = SongDirectory;

        for (auto i : Listing)
        {
            Directory File = i;
            File.Normalize();

            std::wstring Ext = Utility::Widen(File.GetExtension());

            // We want to group charts with the same title together.
            if (ValidBMSExtension(Ext) || Ext == L"bmson")
            {
                BMSSong->SongDirectory = SongDirectory;

                try
                {
                    LoadSong7KFromFilename(File, SongDirectory, BMSSong);
                }
                catch (std::exception &ex)
                {
                    Log::Logf("\nSongLoader::LoadSong7KFromDir(): Exception \"%s\" occurred while loading file \"%s\"\n",
                        ex.what(), File.c_path());
                    Utility::DebugBreak();
                }

                // We found a chart with the same title (and subtitle) already.
                std::string key = BMSSong->SongName + BMSSong->Subtitle;
                if (bmsk.find(key) != bmsk.end())
                {
                    VSRG::Song *oldSng = bmsk[key];

                    if (BMSSong->Difficulties.size()) // BMS charts don't have more than one difficulty anyway.
                        oldSng->Difficulties.push_back(BMSSong->Difficulties[0]);

                    BMSSong->Difficulties.clear();
                    delete BMSSong;
                }
                else // Ah then, don't delete it.
                {
                    bmsk[key] = BMSSong;
                }

                BMSSong = new VSRG::Song;
            }

            if (Ext == L"ojn")
            {
                LoadSong7KFromFilename(File, SongDirectory, OJNSong);
                VSRGUpdateDatabaseDifficulties(DB, OJNSong);
                PushVSRGSong(VecOut, OJNSong);
                OJNSong = new VSRG::Song;
                OJNSong->SongDirectory = SongDirectory;
            }

            if (Ext == L"osu" || Ext == L"fcf")
                LoadSong7KFromFilename(File, SongDirectory, osuSong);

            if (Ext == L"sm" || Ext == L"ssc")
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

        for (auto i : Listing)
        {
            Directory File = i;
            File.Normalize();

            std::wstring Ext = Utility::Widen(File.GetExtension());
            if (VSRGValidExtension(Ext))
            {
                bool cacheRenew = DB->CacheNeedsRenewal(SongDirectory / File.path());
                assert(!cacheRenew);
                int CurrentID = DB->GetSongIDForFile(SongDirectory / File.path(), nullptr);
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
            DB->GetSongInformation7K(*i, New);
            New->SongDirectory = SongDirectory;

            PushVSRGSong(VecOut, New);
            Log::Logf(" ok\n");
        }
    }
}

void SongLoader::GetSongListDC(std::vector<dotcur::Song*> &OutVec, Directory Dir)
{
    std::vector <std::string> Listing;

    Dir.ListDirectory(Listing, Directory::FS_DIR);
    for (auto i = Listing.begin(); i != Listing.end(); ++i)
    {
        Log::Printf("%ls... ", Utility::Widen(*i).c_str());
        LoadSongDCFromDir(Dir.path() + "/" + *i, OutVec);
        Log::Printf("ok\n");
    }
}

void SongLoader::GetSongList7K(std::vector<VSRG::Song*> &OutVec, Directory Dir)
{
    std::vector <std::string> Listing;

    Dir.ListDirectory(Listing, Directory::FS_DIR);
    for (auto i = Listing.begin(); i != Listing.end(); ++i)
    {
        Log::Printf("%ls... ", Utility::Widen(*i).c_str());
        LoadSong7KFromDir(Dir.path() + "/" + *i, OutVec);
        Log::Printf("ok\n");
    }
}

std::shared_ptr<VSRG::Song> SongLoader::LoadFromMeta(const VSRG::Song* Meta, std::shared_ptr<VSRG::Difficulty> &CurrentDiff, Directory *FilenameOut, uint8_t &Index)
{
    std::shared_ptr<VSRG::Song> Out;

    std::string fn = DB->GetDifficultyFilename(CurrentDiff->ID);
    *FilenameOut = fn;

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