#pragma once

#include <ChartGroup.h>
#include "PlayscreenParameters.h"
#include "../structure/GameFilesystem.h"


class SongDatabase;
class Texture;
class Screen;

struct lua_State;

namespace rd
{
    class Difficulty;
    class ScoreKeeper;
}

class Profile;
class PlayerContext;

namespace StormIR {
    class StormIR;
}

class GameState
{
    GameFilesystem Filesystem;
    SongDatabase* Database;

    Texture* StageImage;
    Texture* SongBG;
    std::shared_ptr<otoworm::ChartGroup> SelectedChartGroup;
    std::unique_ptr<StormIR::StormIR> ir;

    struct SPlayerCurrent7K {
        std::shared_ptr<rd::ScoreKeeper> scorekeeper;
        PlayscreenParameters play_parameters;
        std::shared_ptr<otoworm::Chart> active_chart;
        PlayerContext *ctx;
        Profile* profile;
    };

    std::vector<SPlayerCurrent7K> PlayerInfo;

    std::shared_ptr<Screen> RootScreen;
public:

    GameState();
    GameFilesystem& filesystem();
    const GameFilesystem& filesystem() const;
    std::filesystem::path get_skin_script_file(const char* Filename, const std::string& Skin);
    std::shared_ptr<otoworm::ChartGroup> get_selected_chart_group_shared() const;
    std::string get_first_fallback_skin();
    static GameState &get_instance();
    void initialize();

    /* Defines Difficulty/Song/Playscreen/Gamestate
     and defines Global as the Gamestate singleton */
    void initialize_lua(lua_State *L);

    static std::string get_directory_prefix();
    std::string get_skin_prefix();
    static std::string get_skin_prefix(const std::string &skin);
    static std::string get_scripts_directory();
    void set_skin(const std::string& NextSkin);
    Texture* get_skin_image(const std::string& Texture);
    static bool skin_supports_channel_count(int Count);
    std::string get_skin();

    void set_selected_chart_group(std::shared_ptr<otoworm::ChartGroup> chart_group);
    otoworm::ChartGroup *get_selected_chart_group() const;

    Texture* get_song_bg();
    Texture* get_song_stage();

    void start_screen_transition(std::string target) const;
    void exit_current_screen() const;

    std::filesystem::path get_skin_file(const std::string &Name, const std::string &Skin);
    std::filesystem::path get_skin_file(const std::string &Name);
    std::filesystem::path get_fallback_skin_file(const std::string &Name);

    SongDatabase* get_song_database() const;

    void sort_wheel_by(int criteria);

    /* Player-number dependant functions */
    bool player_number_in_bounds(int pn) const;

    void set_player_context(PlayerContext* pc, int pn);

    // VSRG Gauge Type
    int get_current_gauge_type(int pn) const;

    // VSRG score system
    int get_current_score_type(int pn) const;

    // VSRG subsystem
    int get_current_system_type(int pn) const;

    // Note: Returning a shared_ptr causes lua to fail an assertion, since shared_ptr is not registered.
    rd::ScoreKeeper* get_scorekeeper7_k(int pn);
    void set_scorekeeper7_k(std::shared_ptr<rd::ScoreKeeper> other, int pn);

    PlayscreenParameters* get_parameters(int pn);
    otoworm::Chart* get_difficulty(int pn);
    std::shared_ptr<otoworm::Chart> get_difficulty_shared(int pn);
    otoworm::Chart* get_chart(int pn);
    std::shared_ptr<otoworm::Chart> get_chart_shared(int pn);
    void set_chart(std::shared_ptr<otoworm::Chart> chart, int pn);
    int get_player_count() const;
    void submit_score(int pn);

    bool is_song_unlocked(otoworm::ChartGroup *chart_group);
    void unlock_song(otoworm::ChartGroup *chart_group);

    void set_system_folder(const std::string folder);

    void set_root_screen(std::shared_ptr<Screen> root);
    static std::shared_ptr<Screen> get_current_screen();
    std::shared_ptr<Screen> get_next_screen();

    void add_active_profile(const std::string &profile_name);
};
