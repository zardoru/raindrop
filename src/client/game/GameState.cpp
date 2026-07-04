#include <string>
#include <mutex>
#include <future>
#include <rmath.h>


#include <game/ScoreKeeper7K.h>
#include <note_loader.h>

#include "PlayscreenParameters.h"
#include "GameState.h"

#include "../songdb/SongDatabase.h"
#include "../songdb/SongList.h"
#include "../songdb/SongWheel.h"

#include "ScoreDatabase.h"
#include "Profile.h"

#include "Replay7K.h"
#include "PlayerContext.h"

#include "../structure/Screen.h"
#include "../structure/ScreenCustom.h"
#include "../screens/ScreenSelectMusic.h"

#include "ImageLoader.h"

#include "../structure/Configuration.h"
#include <text_and_file_util.h>

#include "../../ir/StormIR.h"
#include "Logging.h"
#include "Audiofile.h"

CfgVar StormIR_AppId    ("AppId", "StormIR");
CfgVar StormIR_ClientKey("ClientKey", "StormIR");
CfgVar StormIR_Username ("Username", "StormIR");
CfgVar StormIR_Password ("Pass", "StormIR");

using namespace rd;

void GameState::set_system_folder(const std::string folder)
{
	Filesystem.set_system_folder(folder);
}

GameState::GameState(): 
	StageImage(nullptr), 
	SongBG(nullptr)
{
    SelectedChartGroup = nullptr;
    Database = nullptr;

    if (!StormIR_AppId.str().empty() && !StormIR_ClientKey.str().empty() ) {
        if (!StormIR_Username.str().empty() && !StormIR_Password.str().empty() ) {
            ir = std::make_unique<StormIR::StormIR>(StormIR_AppId, StormIR_ClientKey);
            if (ir->Login(StormIR_Username, StormIR_Password)) {
                Log::LogPrintf("[IR] Logged into StormIR.\n");
            } else {
                Log::LogPrintf("[IR] Failed to log into StormIR: %s.\n", ir->GetLastError().c_str());
            }
        } else {
            Log::LogPrintf("[IR] StormIR User or Password missing.\n");
        }
    } else {
        Log::LogPrintf("[IR] StormIR appid or clientkey are missing. Could not connect.\n");
    }

	// push the default player
	add_active_profile("machine");
}

GameFilesystem& GameState::filesystem()
{
    return Filesystem;
}

const GameFilesystem& GameState::filesystem() const
{
    return Filesystem;
}

std::filesystem::path GameState::get_skin_script_file(const char* Filename, const std::string& skin)
{
    return Filesystem.get_skin_script_file(Filename, skin);
}

std::shared_ptr<otoworm::ChartGroup> GameState::get_selected_chart_group_shared() const
{
    if (const auto chart_group = SongWheel::GetInstance().GetSelectedChartGroup())
        return chart_group;
    return SelectedChartGroup;
}

std::string GameState::get_first_fallback_skin()
{
    return Filesystem.get_first_fallback_skin();
}

GameState& GameState::get_instance()
{
    static auto* StateInstance = new GameState;
    return *StateInstance;
}

void GameState::set_selected_chart_group(std::shared_ptr<otoworm::ChartGroup> chart_group)
{
    SelectedChartGroup = std::move(chart_group);
}

otoworm::ChartGroup *GameState::get_selected_chart_group() const
{
    return get_selected_chart_group_shared().get();
}

void GameState::start_screen_transition(std::string target) const
{
	if (target.find("custom") == 0) {
		auto res = otoworm::util::token_split(target, ":");
		if (res.size() == 2)
		{
			const auto scr = std::make_shared<ScreenCustom>(res[1]);
			RootScreen->GetTop()->StartTransition(scr);
		}
	}
	else if (target == "songselect") {
		const auto scr = std::make_shared<ScreenSelectMusic>();
		scr->Init();
		RootScreen->GetTop()->StartTransition(scr);
	}
}

void GameState::exit_current_screen() const
{
	RootScreen->GetTop()->Close();
}

std::filesystem::path GameState::get_skin_file(const std::string &Name, const std::string &Skin)
{
    return Filesystem.get_skin_file(Name, Skin);
}

std::filesystem::path GameState::get_skin_file(const std::string& Name)
{
    return Filesystem.get_skin_file(Name);
}

void GameState::initialize()
{
    if (!Database)
    {
        Database = new SongDatabase("rd.db");

        SongBG = new Texture();
        StageImage = new Texture();
    }
}

std::string GameState::get_directory_prefix()
{
	return get_instance().filesystem().get_directory_prefix();
}

std::string GameState::get_skin_prefix()
{
    return Filesystem.get_skin_prefix();
}

std::string GameState::get_skin_prefix(const std::string& skin)
{
    return get_instance().filesystem().get_skin_prefix(skin);
}

void GameState::set_skin(const std::string& Skin)
{
    Filesystem.set_skin(Skin);
}

std::string GameState::get_scripts_directory()
{
    return get_instance().filesystem().get_scripts_directory();
}

SongDatabase* GameState::get_song_database() const
{
    return Database;
}

std::filesystem::path GameState::get_fallback_skin_file(const std::string &Name)
{
    return Filesystem.get_fallback_skin_file(Name);
}

bool GameState::player_number_in_bounds(int pn) const
{
	return pn >= 0 && pn < PlayerInfo.size();
}

void GameState::set_player_context(PlayerContext * pc, int pn)
{
	if (player_number_in_bounds(pn)) {
		PlayerInfo[pn].ctx = pc;
	}
}

int GameState::get_current_gauge_type(int pn) const
{
	if (player_number_in_bounds(pn))
		return PlayerInfo[pn].ctx ? PlayerInfo[pn].ctx->GetCurrentGaugeType() : PlayerInfo[pn].play_parameters.GaugeType;
	return 0;
}

Texture* GameState::get_song_bg()
{
    const auto chart_group = get_selected_chart_group_shared();
	if (chart_group)
	{
		const auto toLoad = chart_group->path / chart_group->background_filename;

		if (std::filesystem::exists(toLoad))
		{
			SongBG->LoadFile(toLoad, true);
			return SongBG;
		}

		// file doesn't exist
		return nullptr;
	}

	// no song selected
	return nullptr;
}

Texture* GameState::get_song_stage()
{
	const auto chart_group = get_selected_chart_group_shared();
	if (chart_group)
	{
		if (PlayerInfo[0].active_chart)
		{
			const auto chart = PlayerInfo[0].active_chart;
			std::filesystem::path File = Database->GetStageFile(static_cast<int>(chart->id));

			// Oh so it's loaded and it's not in the database, fine.
			if (File.wstring().length() == 0 && chart->transient)
				File = chart->transient->stage_file;

			const auto toLoad = chart_group->path / File;

			// ojn files use their cover inside the very ojn
			if (File.extension() == ".ojn")
			{
				size_t read;
				const auto* buf = reinterpret_cast<const unsigned char*>(LoadOJNCover(toLoad, read));
				ImageData data = ImageLoader::GetDataForImageFromMemory(buf, read);
				StageImage->SetTextureData2D(data, true);
				delete[] buf;

				return StageImage;
			}

			if (File.wstring().length() && std::filesystem::exists(toLoad))
			{
				StageImage->LoadFile(toLoad, true);
				return StageImage;
			}

			return nullptr;
		}

		return nullptr;
		// Oh okay, no difficulty assigned.
	}

	// no song selected
	return nullptr;
}

Texture* GameState::get_skin_image(const std::string& Path)
{
    /* Special paths */
    if (Path == "STAGEFILE")
	    return get_song_stage();

    if (Path == "SONGBG")
	    return get_song_bg();

    /* Regular paths */
    if (Path.length())
        return ImageLoader::Load(Filesystem.get_skin_file(Path));

	// no path?
    return nullptr;
}

bool GameState::skin_supports_channel_count(int Count)
{
    return get_instance().filesystem().skin_supports_channel_count(Count);
}

std::string GameState::get_skin()
{
    return Filesystem.get_skin();
}

rd::ScoreKeeper* GameState::get_scorekeeper7_k(int pn)
{
	if (player_number_in_bounds(pn))
		return PlayerInfo[pn].scorekeeper.get();
	else return nullptr;
}

void GameState::set_scorekeeper7_k(std::shared_ptr<rd::ScoreKeeper> other, int pn)
{
    if (player_number_in_bounds(pn))
		PlayerInfo[pn].scorekeeper = other;
}


int GameState::get_current_score_type(int pn) const
{
	if (player_number_in_bounds(pn))
		return PlayerInfo[pn].play_parameters.GetScoringType();
	else
		return 0;
}

int GameState::get_current_system_type(int pn) const
{
	if (player_number_in_bounds(pn))
		return PlayerInfo[pn].ctx ? PlayerInfo[pn].ctx->get_current_system_type() : PlayerInfo[pn].play_parameters.SystemType;
	else
		return 0;
}

void GameState::set_chart(std::shared_ptr<otoworm::Chart> chart, int pn)
{
    if (player_number_in_bounds(pn))
        PlayerInfo[pn].active_chart = std::move(chart);
}

int GameState::get_player_count() const
{
	return PlayerInfo.size();
}

void GameState::submit_score(int pn)
{
	if (!player_number_in_bounds(pn))
		return;

	const auto *player = &PlayerInfo[pn];
	const auto chart = get_chart_shared(pn);
	const auto chart_group = get_selected_chart_group_shared();
	const auto replay = player->ctx->get_replay();

	if (replay.GetEffectiveParameters().Auto)
		return;

	const auto scorekeeper = *player->scorekeeper;
	const auto drift = player->ctx->get_drift();
	const auto joffset = player->ctx->get_judge_offset();
	const auto selected_chart_group = get_selected_chart_group();
	if (!selected_chart_group)
		return;

    auto submitfunc = [=, this] {
        player->profile->Scores.AddScore(
                replay.GetSongHash(),
                replay.GetDifficultyIndex(),
                replay.GetEffectiveParameters(),
                scorekeeper,
                drift,
                joffset
        );

        player->profile->SaveReplay(selected_chart_group, replay);

        if (ir && ir->IsConnected()) {
            Log::LogPrintf("[IR] Submitting score...\n");
            if (ir->SubmitScore(chart_group.get(), chart.get(), replay, scorekeeper)) {
                Log::LogPrintf("[IR] Success.\n");
            } else {
                Log::LogPrintf("[IR] Couldn't submit score: %s\n", ir->GetLastError().c_str());
            }
        }
    };

    std::thread t(submitfunc);
    t.detach();
}

bool GameState::is_song_unlocked(otoworm::ChartGroup * chart_group)
{
	return true;
}

void GameState::unlock_song(otoworm::ChartGroup * chart_group)
{
}

void GameState::set_root_screen(std::shared_ptr<Screen> root)
{
	RootScreen = root;
}

std::shared_ptr<Screen> GameState::get_current_screen()
{
	return std::shared_ptr<Screen>();
}

std::shared_ptr<Screen> GameState::get_next_screen()
{
	return std::shared_ptr<Screen>();
}

void GameState::sort_wheel_by(int criteria)
{
	SongWheel::GetInstance().SortBy(static_cast<ESortCriteria>(criteria));
}

void GameState::add_active_profile(const std::string &profile_name) {
    PlayerInfo.emplace_back();
    auto *new_player = &PlayerInfo.back();
    new_player->profile = new Profile();
    new_player->profile->Load(profile_name);
}
