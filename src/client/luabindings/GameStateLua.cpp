#include <string>
#include <filesystem>

#include <mutex>
#include <glm.h>
#include <rmath.h>

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include <ChartGroup.h>
#include "../game/PlayscreenParameters.h"

#include "../game/GameState.h"

#include "../songdb/SongDatabase.h"
#include "../songdb/SongList.h"
#include "../songdb/SongWheel.h"
#include "../game/PlayscreenParameters.h"
#include "../structure/Configuration.h"

#include <game/ScoreKeeper.h>

enum OBJTYPE
{
    OB_HOLDS,
    OB_NOTES,
    OB_OBJ,
    OB_SCOBJ
};

struct songHelper
{
    template <class T>
    static uint32_t getDifficultyCountForSong(T const *chart_group)
    {
        return chart_group->get_chart_count();
    }


    template <class T>
    static void setDifficultyCountForSong(T *chart_group, uint32_t v)
    {
        return;
    }

    static uint32_t get_chart_count(const otoworm::ChartGroup *chart_group)
    {
        return chart_group->get_chart_count();
    }

    static otoworm::Chart* get_chart(otoworm::ChartGroup *chart_group, uint32_t idx)
    {
        return chart_group->get_chart(idx);
    }

    static std::string get_chart_name(const otoworm::Chart *chart)
    {
        return chart && chart->meta ? chart->meta->name : "";
    }

    static void set_chart_name(otoworm::Chart *chart, std::string name)
    {
        return;
    }

    static std::string get_chart_author(const otoworm::Chart *chart)
    {
        std::string candidate = GameState::get_instance().get_song_database()->GetArtistForDifficulty(chart->id);
        if (!candidate.length() && chart->meta)
            candidate = chart->meta->author;
        return candidate;
    }

    static void set_chart_author(otoworm::Chart *chart, std::string author)
    {
        return;
    }

    static std::string get_chart_genre(const otoworm::Chart *chart)
    {
        return GameState::get_instance().get_song_database()->GetGenreForDifficulty(chart->id);
    }

    static void set_chart_genre(otoworm::Chart *chart, std::string genre)
    {
        return;
    }

    template <class T>
    static std::string getDifficultyAuthor(T const *Diff)
    {
        std::string candidate = GameState::get_instance().get_song_database()->GetArtistForDifficulty(Diff->ID);
        if (!candidate.length())
            candidate = Diff->Author;
        return candidate;
    }

    template <class T>
    static void setDifficultyAuthor(T *Diff, std::string s)
    {
        return;
    }

	template <class T>
	static std::string getDifficultyGenre(T const *Diff)
	{
		auto candidate = GameState::get_instance().get_song_database()->GetGenreForDifficulty(Diff->ID);
		return candidate;
	}


	template <class T>
	static void setDifficultyGenre(T *Diff, std::string s)
	{
		return;
	}

	template <class T>
	static int getLevel(T const *Diff)
	{
		return Diff->level;
	}


	template <class T>
	static void setLevel(T *Diff, int t)
	{
		return;
	}

	template <class T>
	static int GetObjCount(T const* diff)
	{
		return 0;
		/*if (diff->Mode == MODE_VSRG) {
		return;
		}*/
	}

	template <class T>
	static int GetScoreObjCount(T const* diff)
	{
		return 0;
		/*if (diff->Mode == MODE_VSRG) {
		return;
		}*/
	}

	template <class T>
	static void SetObjCount(T* Diff, int s)
	{
		return;
	}


	template <class T>
	static void SetScoreObjCount(T* diff, int s)
	{
		return;
	}
};

PlayscreenParameters* GameState::get_parameters(int pn)
{
	if (player_number_in_bounds(pn))
		return &PlayerInfo[pn].play_parameters;
	else return nullptr;
}

otoworm::Chart * GameState::get_difficulty(int pn)
{
    auto chart = get_difficulty_shared(pn);
    return chart.get();
}

std::shared_ptr<otoworm::Chart> GameState::get_difficulty_shared(int pn)
{
    return get_chart_shared(pn);
}

otoworm::Chart *GameState::get_chart(int pn)
{
    auto chart = get_chart_shared(pn);
    return chart.get();
}

std::shared_ptr<otoworm::Chart> GameState::get_chart_shared(int pn)
{
    if (player_number_in_bounds(pn) && PlayerInfo[pn].active_chart)
        return PlayerInfo[pn].active_chart;

    auto group = get_selected_chart_group_shared();
    if (!group)
        return nullptr;

    const auto index = SongWheel::get_instance().get_difficulty();
    if (index < group->charts.size())
        return group->charts[index];

    return group->charts.empty() ? nullptr : group->charts.front();
}




/// rd Data types. Generally inert.
// @engineclass GameState
void GameState::initialize_lua(lua_State *L)
{
	LuaManager l(L);

	rd::SetupScorekeeperLuaInterface(L);

	luabridge::getGlobalNamespace(L)
		/// Base Song class.
		/// @type Song
		.beginClass <otoworm::ChartGroup>("Song")
		/// The song's title.
		// @roproperty Title 
		.addData("Title", &otoworm::ChartGroup::title, false)
		/// The song's author.
		// @roproperty Author 
		.addData("Author", &otoworm::ChartGroup::artist, false)
		/// The song's subtitle.
		// @roproperty Subtitle 
		.addData("Subtitle", &otoworm::ChartGroup::subtitle, false)
		/// The song's database ID.
		// @roproperty ID 
		.addData("ID", &otoworm::ChartGroup::id, false)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// The Base Difficulty class.
		/// @type Difficulty
		.beginClass <otoworm::Chart>("Difficulty")
		/// Duration of the difficulty, in seconds.
		// @roproperty Duration
		.addData("Duration", &otoworm::Chart::duration, false)
		/// Difficulty name.
		// @roproperty Name
		.addProperty("Name", &songHelper::get_chart_name, &songHelper::set_chart_name)
		/// Offset.
		// @roproperty Offset
		.addData("Offset", &otoworm::Chart::offset, false)
		/// Object count.
		// @roproperty Objects
		.addProperty("Objects", 
			&songHelper::GetObjCount<otoworm::Chart>,
			&songHelper::SetObjCount<otoworm::Chart>)
		/// Count of objects that matter for score.
		// @roproperty ScoreObjects
		.addProperty("ScoreObjects",
			&songHelper::GetScoreObjCount<otoworm::Chart>,
			&songHelper::SetScoreObjCount<otoworm::Chart>)
		/// Difficulty's Author.
		// @roproperty Author
		.addProperty("Author", 
			&songHelper::get_chart_author,
			&songHelper::set_chart_author)
		/// Difficulty's Genre.
		// @roproperty Genre
		.addProperty("Genre", 
			&songHelper::get_chart_genre,
			&songHelper::set_chart_genre)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// VSRG specific difficulty class.
		/// @type Difficulty7K
		.deriveClass <otoworm::Chart, otoworm::Chart>("Difficulty7K")
		/// Level, as informed by the loader.
		// @roproperty Level
		.addProperty("Level", 
			&songHelper::getLevel<otoworm::Chart>,
			&songHelper::setLevel<otoworm::Chart>)
		/// Effective channels in use.
		// @roproperty Channels
		.addData("Channels", &otoworm::Chart::channels, false)
		.endClass();


	luabridge::getGlobalNamespace(L)
		/// Song class, specific for VSRG.
		/// @type Song
		.beginClass <otoworm::ChartGroup>("Song")
		/// Difficulty count
		// @roproperty DifficultyCount
		.addProperty("DifficultyCount", &songHelper::get_chart_count,
			&songHelper::setDifficultyCountForSong<otoworm::ChartGroup>)
		/// Get a difficulty, by index. Can return nil.
		// @function GetDifficulty
		// @param index The difficulty index.
		// @return A Difficulty. Can be nil.
		.addFunction("GetDifficulty", &songHelper::get_chart)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// A request of playscreen parameters.
		/// @type PlayscreenParameters
		.beginClass <PlayscreenParameters>("PlayscreenParameters")
		/// Request upscroll.
		// @property Upscroll
		.addData("Upscroll", &PlayscreenParameters::upscroll)
		/// Request to disable failing.
		// @property NoFail
		.addData("NoFail", &PlayscreenParameters::no_fail)
		/// Request to autoplay.
		// @property Autoplay
		.addData("Autoplay", &PlayscreenParameters::Auto)
		/// Hidden mode request. 
		// @enumproperty HiddenMode
		// @param 0 None 
		// @param 1 Sudden
		// @param 2 Hidden
		// @param 3 Flashlight
		.addData("HiddenMode", &PlayscreenParameters::hidden_mode)
		//.addData("Rate", &PlayscreenParameters::Rate)
		/// Request a random permutation of lanes.
		// @property Random
		.addData("Random", &PlayscreenParameters::random)
		/// Request a specific gauge type.
		// @enumproperty GaugeType
		// @param 0 Auto
		// @param 1 Groove gauge
		// @param 2 Survival gauge
		// @param 3 ExHard gauge
		// @param 4 Death gauge
		// @param 5 Easy gauge
		// @param 6 Stepmania gauge
		// @param 7 No Recovery gauge
		// @param 8 O2Jam Gauge
		.addData("GaugeType", &PlayscreenParameters::gauge_type)
		/// Request a specific game type.
		// @enumproperty SystemType
		// @param 0 Auto
		// @param 1 BMS
		// @param 2 osu!mania
		// @param 3 o2jam
		// @param 4 Stepmania
		// @param 5 Raindrop
		// @param 6 RDAC
		.addData("SystemType", &PlayscreenParameters::system_type)
		/// Whether to treat input desired speed as a green number
		// @property GreenNumber
		.addData("GreenNumber", &PlayscreenParameters::green_number)
		/// Whether to enable extended W0 judge
		// @property UseW0
		.addData("UseW0", &PlayscreenParameters::use_w0)
        .addData("SpeedType", &PlayscreenParameters::speed_type)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// Global game status. Available in most places.
		/// @type GameState
		.beginClass <GameState>("GameState")
		/// Returns currently selected song.
		// @function GetSelectedSong
		// @return The currently selected song.
		.addFunction("GetSelectedSong", &GameState::get_selected_chart_group)
		/// Returns the player's current difficulty.
		// @function GetDifficulty
		// @param pn The player number.
		// @return The player's Difficulty7K.
		.addFunction("GetDifficulty", &GameState::get_difficulty)
		/// Returns the player's current scorekeeper.
		// @function GetScorekeeper7K
		// @param pn The player number.
		// @return The player's @{ScoreKeeper7K}.
		.addFunction("GetScorekeeper7K", &GameState::get_scorekeeper7_k)
		/// Returns the player's currently requested parameters.
		// @function GetParameters
		// @param pn The player number.
		// @return The player's PlayscreenParameters.
		.addFunction("GetParameters", &GameState::get_parameters)
		/// Returns the player's currently effective Gauge Type.
		// @function GetCurrentGaugeType
		// @param pn The player number.
		// @return A player's current gauge type.
		.addFunction("GetCurrentGaugeType", &GameState::get_current_gauge_type)
		/// Returns the player's currently effective score type.
		// @function GetCurrentScoreType
		// @param pn The player number.
		// @return A player's current score type.
		.addFunction("GetCurrentScoreType", &GameState::get_current_score_type)
		/// Returns the player's currently effective system type.
		// @function GetCurrentSystemType
		// @param pn The player number.
		// @return A player's current system type.
		.addFunction("GetCurrentSystemType", &GameState::get_current_system_type)
		/// Sort the wheel using a criteria.
		// @function SortWheelBy
		// @param crit A criteria. Currently an enum.
		.addFunction("SortWheelBy", &GameState::sort_wheel_by)
		/// Starts a new screen. Use "custom:filename" to use your own script.
		// filename is a path relative to the program's working directory.
		// You can use "songselect" as well.
		// @function StartScreen
		// @param screen The string describing the screen to transition to.
		.addFunction("StartScreen", &GameState::start_screen_transition)
		/// Pops the current screen and goes back up one level.
		// @function ExitScreen
		.addFunction("ExitScreen", &GameState::exit_current_screen)
		.endClass();

	luabridge::push(L, this);
	lua_setglobal(L, "Global");

	luabridge::getGlobalNamespace(L)
		/// Always available. Effectively a namespace.
		/// @type System
		.beginNamespace("System")
		/// Set a value from config.ini.
		// @function SetConfig 
		// @param name The configuration variable name.
		// @param value The new value.
		// @param[opt] section The section of the variable. If omitted, is "Global".
		.addFunction("SetConfig", Configuration::SetConfig)
		/// Get a value from config.ini as a float.
		// @function ReadConfigF
		// @param name The configuration variable name.
		// @param[opt] section The section of the variable. If omitted, is "Global".
		.addFunction("ReadConfigF", Configuration::GetConfigf)
		/// Get a value from config.ini as a string.
		// @function ReadConfigS
		// @param name The configuration variable name.
		// @param[opt] section The section of the variable. If omitted, is "Global".
		.addFunction("ReadConfigS", Configuration::GetConfigs)
		/// Reload all configuration files.
		// @function ReloadConfiguration
		.addFunction("ReloadConfiguration", Configuration::Reload)
		.endNamespace();
}
