#include <string>
#include <mutex>
#include <future>
#include <rmath.h>


#include <game/ScoreKeeper.h>
#include <note_loader.h>

#include "PlayscreenParameters.h"
#include "GameState.h"

#include "../songdb/SongDatabase.h"
#include "../songdb/SongList.h"
#include "../songdb/SongWheel.h"

#include "ScoreDatabase.h"
#include "Profile.h"

#include "Replay.h"
#include "PlayerContext.h"

#include "../structure/Screen.h"
#include "../structure/ScreenCustom.h"
#include "../screens/ScreenSelectMusic.h"

#include "TextureCollection.h"

#include "../structure/Configuration.h"
#include <text_and_file_util.h>

#include "../ir/StormIR.h"
#include "Logging.h"
#include "Audiofile.h"

CfgVar StormIR_AppId    ("AppId", "StormIR");
CfgVar StormIR_ClientKey("ClientKey", "StormIR");
CfgVar StormIR_Username ("Username", "StormIR");
CfgVar StormIR_Password ("Pass", "StormIR");

using namespace rd;

void GameState::set_system_folder(const std::string folder)
{
	filesystem_.set_system_folder(folder);
}

void GameState::login_internet_ranking() {
    if (StormIR_AppId.str().empty() || StormIR_ClientKey.str().empty() ) {
        Log::LogPrintf("[IR] StormIR appid or clientkey are missing. Could not connect.\n");
        return;
    }

    if (StormIR_Username.str().empty() || StormIR_Password.str().empty() ) {
        Log::LogPrintf("[IR] StormIR User or Password missing.\n");
        return;
    }

    ir_ = std::make_unique<StormIR::StormIR>(StormIR_AppId, StormIR_ClientKey);
    if (ir_->login(StormIR_Username, StormIR_Password)) {
        Log::LogPrintf("[IR] Logged into StormIR.\n");
    } else {
        Log::LogPrintf("[IR] Failed to log into StormIR: %s.\n", ir_->get_last_error().c_str());
    }
}

GameState::GameState():
	stage_image_(nullptr),
	song_bg_(nullptr)
{
    selected_chart_group_ = nullptr;
    database_ = nullptr;

    login_internet_ranking();

	// push the default player
	add_active_profile("machine");
}

GameFilesystem& GameState::filesystem()
{
    return filesystem_;
}

const GameFilesystem& GameState::filesystem() const
{
    return filesystem_;
}

std::filesystem::path GameState::get_skin_script_file(const char* Filename, const std::string& skin) const {
    return filesystem_.get_skin_script_file(Filename, skin);
}

std::shared_ptr<otoworm::ChartGroup> GameState::get_selected_chart_group_shared() const
{
    if (const auto chart_group = SongWheel::get_instance().get_selected_chart_group())
        return chart_group;
    return selected_chart_group_;
}

std::string GameState::get_first_fallback_skin() const {
    return filesystem_.get_first_fallback_skin();
}

GameState& GameState::get_instance()
{
    static auto* StateInstance = new GameState;
    return *StateInstance;
}

void GameState::set_selected_chart_group(std::shared_ptr<otoworm::ChartGroup> chart_group)
{
    selected_chart_group_ = std::move(chart_group);
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
            const auto scr = std::make_shared<ScreenCustom>(root_screen_->get_window(), res[1]);
			root_screen_->get_top()->start_transition(scr);
		}
	}
	else if (target == "songselect") {
        const auto scr = std::make_shared<ScreenSelectMusic>(root_screen_->get_window());
		scr->init();
		root_screen_->get_top()->start_transition(scr);
	}
}

void GameState::exit_current_screen() const
{
	root_screen_->get_top()->close();
}

std::filesystem::path GameState::get_skin_file(const std::string &name, const std::string &skin)
{
    return filesystem_.get_skin_file(name, skin);
}

std::filesystem::path GameState::get_skin_file(const std::string& name) const {
    return filesystem_.get_skin_file(name);
}

void GameState::initialize()
{
    if (!database_)
    {
        database_ = new SongDatabase("rd.db");

        song_bg_ = new Texture2D();
        stage_image_ = new Texture2D();
    }
}

std::string GameState::get_directory_prefix()
{
	return get_instance().filesystem().get_directory_prefix();
}

std::string GameState::get_skin_prefix() const {
    return filesystem_.get_skin_prefix();
}

std::string GameState::get_skin_prefix(const std::string& skin)
{
    return get_instance().filesystem().get_skin_prefix(skin);
}

void GameState::set_skin(const std::string& Skin)
{
    filesystem_.set_skin(Skin);
}

std::string GameState::get_scripts_directory()
{
    return get_instance().filesystem().get_scripts_directory();
}

SongDatabase* GameState::get_song_database() const
{
    return database_;
}

std::filesystem::path GameState::get_fallback_skin_file(const std::string &name) const {
    return filesystem_.get_fallback_skin_file(name);
}

bool GameState::player_number_in_bounds(int pn) const
{
	return pn >= 0 && pn < player_info_.size();
}

void GameState::set_player_context(PlayerContext * pc, int pn)
{
	if (player_number_in_bounds(pn)) {
		player_info_[pn].ctx = pc;
	}
}

int GameState::get_current_gauge_type(int pn) const
{
	if (player_number_in_bounds(pn))
		return player_info_[pn].ctx ? player_info_[pn].ctx->get_current_gauge_type() : player_info_[pn].play_parameters.gauge_type;
	return 0;
}

Texture2D* GameState::get_song_bg() const {
    const auto chart_group = get_selected_chart_group_shared();
	if (!chart_group) {
        // no song selected
        return nullptr;
    }

    const auto toLoad = chart_group->path / chart_group->background_filename;

    if (std::filesystem::exists(toLoad))
    {
        song_bg_->load_file(toLoad, true);
        return song_bg_;
    }

    // file doesn't exist
    return nullptr;
}

Texture2D* GameState::get_song_stage() const {
	const auto chart_group = get_selected_chart_group_shared();
	if (!chart_group) {
        // no song selected
        return nullptr;
    }

    if (!player_info_[0].active_chart) {
        return nullptr;
        // Oh okay, no difficulty assigned.
    }

    const auto chart = player_info_[0].active_chart;
    std::filesystem::path file = database_->GetStageFile(static_cast<int>(chart->id));

    // Oh so it's loaded and it's not in the database, fine.
    if (file.wstring().empty() && chart->transient)
        file = chart->transient->stage_file;

    const auto to_load = chart_group->path / file;

    // ojn files use their cover inside the very ojn
    if (file.extension() == ".ojn")
    {
        size_t read;
        const auto buf = load_ojn_cover(to_load, read);
        const ImageData2d data = TextureCollection::get_data_for_image_from_memory(buf.get(), read);
        stage_image_->set_texture_data_2d(data, true);

        return stage_image_;
    }

    if (!file.wstring().empty() && std::filesystem::exists(to_load))
    {
        stage_image_->load_file(to_load, true);
        return stage_image_;
    }

    return nullptr;
}

Texture2D* GameState::get_skin_image(const std::string& Path)
{
    /* Special paths */
    if (Path == "STAGEFILE")
	    return get_song_stage();

    if (Path == "SONGBG")
	    return get_song_bg();

    /* Regular paths */
    if (Path.length())
        return TextureCollection::load(filesystem_.get_skin_file(Path));

	// no path?
    return nullptr;
}

bool GameState::skin_supports_channel_count(int Count)
{
    return get_instance().filesystem().skin_supports_channel_count(Count);
}

std::string GameState::get_skin() const {
    return filesystem_.get_skin();
}

rd::ScoreKeeper* GameState::get_scorekeeper7_k(int pn) const {
	if (player_number_in_bounds(pn))
		return player_info_[pn].scorekeeper.get();
	else return nullptr;
}

void GameState::set_scorekeeper7_k(std::shared_ptr<rd::ScoreKeeper> other, int pn)
{
    if (player_number_in_bounds(pn))
		player_info_[pn].scorekeeper = other;
}


int GameState::get_current_score_type(int pn) const
{
	if (player_number_in_bounds(pn))
		return player_info_[pn].play_parameters.get_scoring_type();
	else
		return 0;
}

int GameState::get_current_system_type(int pn) const
{
	if (player_number_in_bounds(pn))
		return player_info_[pn].ctx ? player_info_[pn].ctx->get_current_system_type() : player_info_[pn].play_parameters.system_type;
	else
		return 0;
}

void GameState::set_chart(std::shared_ptr<otoworm::Chart> chart, int pn)
{
    if (player_number_in_bounds(pn))
        player_info_[pn].active_chart = std::move(chart);
}

int GameState::get_player_count() const
{
	return player_info_.size();
}

void GameState::submit_score(int pn)
{
	if (!player_number_in_bounds(pn))
		return;

	const auto *player = &player_info_[pn];
	const auto chart = get_chart_shared(pn);
	const auto chart_group = get_selected_chart_group_shared();
	const auto replay = player->ctx->get_replay();

	if (replay.get_effective_parameters().Auto)
		return;

	const auto scorekeeper = *player->scorekeeper;
	const auto drift = player->ctx->get_drift();
	const auto joffset = player->ctx->get_judge_offset();
	const auto selected_chart_group = get_selected_chart_group();
	if (!selected_chart_group)
		return;

    std::thread([=, this] {
        player->profile->scores.add_score(
                replay.get_song_hash(),
                replay.get_difficulty_index(),
                replay.get_effective_parameters(),
                scorekeeper,
                drift,
                joffset
        );

        player->profile->save_replay(selected_chart_group, replay);

        if (ir_ && ir_->is_connected()) {
            Log::LogPrintf("[IR] Submitting score...\n");
            if (ir_->submit_score(chart_group.get(), chart.get(), replay, scorekeeper)) {
                Log::LogPrintf("[IR] Success.\n");
            } else {
                Log::LogPrintf("[IR] Couldn't submit score: %s\n", ir_->get_last_error().c_str());
            }
        }
    }).detach();
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
	root_screen_ = root;
}

Screen* GameState::get_current_screen() const {
	return root_screen_->get_top();
}

std::shared_ptr<Screen> GameState::get_next_screen()
{
	return std::shared_ptr<Screen>();
}

void GameState::sort_wheel_by(int criteria)
{
	SongWheel::get_instance().sort_by(static_cast<ESortCriteria>(criteria));
}

void GameState::add_active_profile(const std::string &profile_name) {
    player_info_.emplace_back();
    auto *new_player = &player_info_.back();
    new_player->profile = new Profile();
    new_player->profile->load(profile_name);
}
