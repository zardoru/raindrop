#include <string>

#include "Logging.h"

#include <array>
#include <string_view>
#include <note_loader.h>
#include <text_and_file_util.h>
#include <cassert>
#include <utility>
#include "SongDatabase.h"
#include "SongLoader.h"
#include "../structure/Configuration.h"


constexpr auto VSRG_EXTENSIONS = std::array{
    std::wstring_view(L".bms"),
    std::wstring_view(L".bme"),
    std::wstring_view(L".bml"),
    std::wstring_view(L".pms"),
    std::wstring_view(L".sm"),
    std::wstring_view(L".osu"),
    std::wstring_view(L".ft2"),
    std::wstring_view(L".ojn"),
    std::wstring_view(L".ssc"),
    std::wstring_view(L".bmson"),
};

constexpr auto BMS_EXTENSIONS = std::array{
    std::wstring_view(L".bms"),
    std::wstring_view(L".bme"),
    std::wstring_view(L".bml"),
    std::wstring_view(L".pms"),
};

SongLoader::SongLoader(SongDatabase* Database)
{
    DB = Database;
}

bool VSRGValidExtension(const std::wstring &s)
{
    for (auto ext : VSRG_EXTENSIONS)
        if (s == ext)
            return true;

    return false;
}

bool ValidBMSExtension(const std::wstring &s)
{
    for (auto ext : BMS_EXTENSIONS)
        if (s == ext)
            return true;

    return false;
}



std::shared_ptr<otoworm::ChartGroup> LoadChartGroupFromFilename(const std::filesystem::path& Filename)
{
    // no extension
    if (!Filename.has_extension() || !VSRGValidExtension(Filename.extension().wstring()))
        return nullptr;

	auto fn = Filename;

    Log::LogPrintf("SongLoader: Load %ls from disk...", fn.wstring().c_str());
    try
    {
        auto chart_group = otoworm::load_song_from_file(fn);
        if (!chart_group)
            return nullptr;

        Log::LogPrintf(" ok\n");
        return chart_group;
    }
    catch (std::exception &e)
    {
        Log::LogPrintf("SongLoader: Failure loading. Reason: %s \n", e.what());
    }

    return nullptr;
}



void AddSongToList(std::vector<std::shared_ptr<otoworm::ChartGroup>> &VecOut, std::shared_ptr<otoworm::ChartGroup> chart_group)
{
    if (chart_group && !chart_group->charts.empty())
        VecOut.push_back(std::move(chart_group));
}

CfgVar NoFileGrouping("NoFileGrouping");

void SongLoader::LoadBMS(
        std::shared_ptr<otoworm::ChartGroup> &bms_group,
        std::filesystem::path file,
        std::map<std::string, std::shared_ptr<otoworm::ChartGroup>> &bmsk,
        std::vector<std::shared_ptr<otoworm::ChartGroup>> &VecOut)
{
	try
	{
        auto loaded_group = LoadChartGroupFromFilename(file);
        if (!loaded_group)
            return;

        if (!bms_group)
            bms_group = loaded_group;
        else
            bms_group->charts.insert(
                    bms_group->charts.end(),
                    loaded_group->charts.begin(),
                    loaded_group->charts.end());
	}
	catch (std::exception &ex)
	{
		Log::Logf("\nSongLoader::LoadChartGroupsFromDir(): Exception \"%s\" occurred while loading file \"%ls\"\n",
			ex.what(), file.wstring().c_str());
		otoworm::util::debug_break();
	}

	// We found a chart with the same title (and subtitle) already.

	if (!NoFileGrouping) {
		std::string key;
		//if (Configuration::GetConfigf("SeparateBySubtitle"))
			key = bms_group->title + bms_group->subtitle;
		//else
		//	key = bms_group->title;

		if (bmsk.find(key) != bmsk.end())
		{
			auto old_group = bmsk[key];

			if (!bms_group->charts.empty()) // BMS charts don't have more than one difficulty anyway.
				old_group->charts.push_back(bms_group->charts[0]);
		}
		else // Ah then, don't delete it.
		{
			bmsk[key] = bms_group;
		}

		bms_group = nullptr;
	}
	else
	{
		DB->AssociateSong(bms_group.get());
		AddSongToList(VecOut, bms_group);
		bms_group = nullptr;
	}
}

std::vector<std::filesystem::path> pathlist(32);

void SongLoader::LoadChartGroupsFromDir(std::filesystem::path songPath, std::vector<std::shared_ptr<otoworm::ChartGroup>> &VecOut)
{
	if (!std::filesystem::is_directory(songPath))
		return;

    std::filesystem::path song_directory = std::filesystem::absolute(songPath);

    /*
        Procedure:
        1.- Check all files if cache needs to be renewed or created.
        2.- If it needs to, load the song again.
        3.- If it loaded the song for either reason, rewrite the difficulty cache.
        4.- If it does not need to be renewed or created, just read the metadata and leave it like that.
    */

    bool renew_cache = false;

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
    for (auto &entry: std::filesystem::directory_iterator(song_directory))
    {
		const auto &File = entry.path();

        /*
            Some people leave nameless, blank .bms files on their folder.
            This causes the cache to do a full reload, so we check if
            we should just ignore this file.
            It'll be loaded in any case, but not considered for cache.
        */
        auto ext = File.extension().wstring();
		if (VSRGValidExtension(ext))
		{
			if (DB->CacheNeedsRenewal(File)) {
				Log::LogPrintf("File '%ls' needs renewal.\n", File.wstring().c_str());
				renew_cache = true;
			}

			pathlist.push_back(File);
		}
    }

    // Files were modified- we have to reload the charts.
    if (renew_cache)
    {
		std::map<std::string, std::shared_ptr<otoworm::ChartGroup>> bmsk;

		// These may or not be grouped together.
		std::shared_ptr<otoworm::ChartGroup> bms_group;

		// Every OJN gets its own chart group.
		std::shared_ptr<otoworm::ChartGroup> ojn_group;

		// osu!mania charts are packed together, with FTB charts.
		std::shared_ptr<otoworm::ChartGroup> osu_group;

		// Stepmania charts get their own chart groups too.
		std::shared_ptr<otoworm::ChartGroup> sm_group;

        for (auto &entry: pathlist)
        {
			// get extension
			auto Ext = entry.extension().wstring();

            // We want to group charts with the same title together.
            if (ValidBMSExtension(Ext) || Ext == L".bmson")
            {
				LoadBMS(bms_group, entry, bmsk, VecOut);
            }

			// .ft2 doesn't need its own entry. 
            if (Ext == L".ojn" || Ext == L".ft2")
            {
                ojn_group = LoadChartGroupFromFilename(entry);
                DB->AssociateSong(ojn_group.get());
                AddSongToList(VecOut, ojn_group);
                ojn_group = nullptr;
            }

			// Add them all to the same song.
            if (Ext == L".osu")
            {
                auto loaded_group = LoadChartGroupFromFilename(entry);
                if (!osu_group)
                    osu_group = loaded_group;
                else if (loaded_group)
                    osu_group->charts.insert(
                            osu_group->charts.end(),
                            loaded_group->charts.begin(),
                            loaded_group->charts.end());
            }

			// Same as before.
            if (Ext == L".sm" || Ext == L".ssc")
            {
                sm_group = LoadChartGroupFromFilename(entry);
                DB->AssociateSong(sm_group.get());
                AddSongToList(VecOut, sm_group);
                sm_group = nullptr;
            }
        }

        // AddSongToList() handles the cleanup.
        for (auto & i : bmsk)
        {
            DB->AssociateSong(i.second.get());
            AddSongToList(VecOut, i.second);
        }

        DB->AssociateSong(ojn_group.get());
        AddSongToList(VecOut, ojn_group);

        DB->AssociateSong(osu_group.get());
        AddSongToList(VecOut, osu_group);

        DB->AssociateSong(sm_group.get());
        AddSongToList(VecOut, sm_group);
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
            auto new_group = std::make_shared<otoworm::ChartGroup>();
            Log::Logf("Song ID %d load from cache...", i);
			try {
				DB->GetSongInformation(i, new_group.get());
				new_group->path = song_directory;

				// make sure it's a well-formed directory on debug
				assert(std::filesystem::exists(new_group->path));

				AddSongToList(VecOut, new_group);
				Log::Logf(" ok\n");
			}
			catch (std::exception &e) {
				Log::Logf("Error loading from cache: %s\n", e.what());
			}
        }
    }
}

void SongLoader::GetChartGroupList(std::vector<std::shared_ptr<otoworm::ChartGroup>> &OutVec,std::filesystem::path Dir)
{
    for (const auto& entry : std::filesystem::directory_iterator(Dir))
    {
        const auto& i = entry.path();
		Log::Printf("%s... ", i.c_str());
        LoadChartGroupsFromDir(i, OutVec);
        Log::Printf("ok\n");
    }
}

std::shared_ptr<otoworm::ChartGroup> SongLoader::LoadFromMeta(
        const int meta_song_id,
        const std::shared_ptr<otoworm::Chart>& current_chart,
        std::filesystem::path &FilenameOut,
        uint8_t &Index)
{
    const auto chart_id = current_chart ? static_cast<int>(current_chart->id) : -1;
    std::filesystem::path fn = DB->GetDifficultyFilename(chart_id);
    FilenameOut = fn;

	Log::LogPrintf("Loading chart from meta ID %i from %ls\n", meta_song_id, fn.wstring().c_str());
    auto out = LoadChartGroupFromFilename(fn);
    if (!out) return nullptr;
	
    Index = 0;
    /* Find out Difficulty IDs to the recently loaded song's difficulty! */
    bool difficulty_found = false;
    for (const auto& chart : out->charts)
    {
        DB->InsertOrUpdateDifficulty(meta_song_id, chart.get());
        if (static_cast<int>(chart->id) == chart_id) // We've got a match; move onward.
        {
            difficulty_found = true;
            break; // We're done here, we've found the difficulty we were trying to load
        }
        Index++;
    }

    if (!difficulty_found)
        return nullptr;

    return out;
}
