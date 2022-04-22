#include <string>
#include <filesystem>

#include <mutex>
#include <glm.h>
#include <rmath.h>

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include <game/Song.h>
#include "../game/PlayscreenParameters.h"

#include "../game/GameState.h"

#include "../songdb/SongDatabase.h"
#include "../songdb/SongList.h"
#include "../songdb/SongWheel.h"
#include "../game/PlayscreenParameters.h"
#include "../structure/Configuration.h"

#include <game/ScoreKeeper7K.h>

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
    static uint32_t getDifficultyCountForSong(T const *Sng)
    {
        return Sng->Difficulties.size();
    }


    template <class T>
    static void setDifficultyCountForSong(T *Sng, uint32_t v)
    {
        return;
    }

    template <class T>
    static std::string getDifficultyAuthor(T const *Diff)
    {
        std::string candidate = GameState::GetInstance().GetSongDatabase()->GetArtistForDifficulty(Diff->ID);
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
		auto candidate = GameState::GetInstance().GetSongDatabase()->GetGenreForDifficulty(Diff->ID);
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
		return Diff->Level;
	}


	template <class T>
	static void setLevel(T *Diff, int t)
	{
		return;
	}

    template <class T, class Q>
    static Q* getDifficulty(T *Sng, uint32_t idx)
    {
        if (idx < 0 || idx >= getDifficultyCountForSong(Sng))
            return NULL;

        Q* diff = (Q*)Sng->Difficulties[idx];
        return diff;
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

PlayscreenParameters* GameState::GetParameters(int pn)
{
	if (PlayerNumberInBounds(pn))
		return &PlayerInfo[pn].play_parameters;
	else return nullptr;
}

rd::Difficulty * GameState::GetDifficulty(int pn)
{
	if(GetSelectedSong())
		return ((rd::Song*)GetSelectedSong())->GetDifficulty(SongWheel::GetInstance().GetDifficulty());
	return nullptr;
}

std::shared_ptr<rd::Difficulty> GameState::GetDifficultyShared(int pn)
{
	if (PlayerNumberInBounds(pn) && PlayerInfo[pn].active_difficulty)
		return PlayerInfo[pn].active_difficulty;
	else {
		return ((rd::Song*)GetSelectedSong())->Difficulties[SongWheel::GetInstance().GetDifficulty()];
	}

	return nullptr;
}




/// rd Data types. Generally inert.
// @engineclass GameState
void GameState::InitializeLua(lua_State *L)
{
	LuaManager l(L);

	rd::SetupScorekeeperLuaInterface(L);

	luabridge::getGlobalNamespace(L)
		/// Base Song class.
		/// @type Song
		.beginClass <rd::Song>("Song")
		/// The song's title.
		// @roproperty Title 
		.addData("Title", &rd::Song::Title, false)
		/// The song's author.
		// @roproperty Author 
		.addData("Author", &rd::Song::Artist, false)
		/// The song's subtitle.
		// @roproperty Subtitle 
		.addData("Subtitle", &rd::Song::Subtitle, false)
		/// The song's database ID.
		// @roproperty ID 
		.addData("ID", &rd::Song::ID, false)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// The Base Difficulty class.
		/// @type Difficulty
		.beginClass <rd::Difficulty>("Difficulty")
		/// Duration of the difficulty, in seconds.
		// @roproperty Duration
		.addData("Duration", &rd::Difficulty::Duration, false)
		/// Difficulty name.
		// @roproperty Name
		.addData("Name", &rd::Difficulty::Name, false)
		/// Offset.
		// @roproperty Offset
		.addData("Offset", &rd::Difficulty::Offset, false)
		/// Object count.
		// @roproperty Objects
		.addProperty("Objects", 
			&songHelper::GetObjCount<rd::Difficulty>,
			&songHelper::SetObjCount<rd::Difficulty>)
		/// Count of objects that matter for score.
		// @roproperty ScoreObjects
		.addProperty("ScoreObjects",
			&songHelper::GetScoreObjCount<rd::Difficulty>,
			&songHelper::SetScoreObjCount<rd::Difficulty>)
		/// Difficulty's Author.
		// @roproperty Author
		.addProperty("Author", 
			&songHelper::getDifficultyAuthor<rd::Difficulty>,
			&songHelper::setDifficultyAuthor <rd::Difficulty>)
		/// Difficulty's Genre.
		// @roproperty Genre
		.addProperty("Genre", 
			&songHelper::getDifficultyGenre<rd::Difficulty>,
			&songHelper::setDifficultyGenre<rd::Difficulty>)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// VSRG specific difficulty class.
		/// @type Difficulty7K
		.deriveClass <rd::Difficulty, rd::Difficulty>("Difficulty7K")
		/// Level, as informed by the loader.
		// @roproperty Level
		.addProperty("Level", 
			&songHelper::getLevel<rd::Difficulty>,
			&songHelper::setLevel<rd::Difficulty>)
		/// Effective channels in use.
		// @roproperty Channels
		.addData("Channels", &rd::Difficulty::Channels, false)
		.endClass();


	luabridge::getGlobalNamespace(L)
		/// Song class, specific for VSRG.
		/// @type Song
		.beginClass <rd::Song>("Song")
		/// Difficulty count
		// @roproperty DifficultyCount
		.addProperty("DifficultyCount", &songHelper::getDifficultyCountForSong<rd::Song>,
			&songHelper::setDifficultyCountForSong<rd::Song>)
		/// Get a difficulty, by index. Can return nil.
		// @function GetDifficulty
		// @param index The difficulty index.
		// @return A Difficulty. Can be nil.
		.addFunction("GetDifficulty", &rd::Song::GetDifficulty)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// A request of playscreen parameters.
		/// @type PlayscreenParameters
		.beginClass <PlayscreenParameters>("PlayscreenParameters")
		/// Request upscroll.
		// @property Upscroll
		.addData("Upscroll", &PlayscreenParameters::Upscroll)
		/// Request to disable failing.
		// @property NoFail
		.addData("NoFail", &PlayscreenParameters::NoFail)
		/// Request to autoplay.
		// @property Autoplay
		.addData("Autoplay", &PlayscreenParameters::Auto)
		/// Hidden mode request. 
		// @enumproperty HiddenMode
		// @param 0 None 
		// @param 1 Sudden
		// @param 2 Hidden
		// @param 3 Flashlight
		.addData("HiddenMode", &PlayscreenParameters::HiddenMode)
		//.addData("Rate", &PlayscreenParameters::Rate)
		/// Request a random permutation of lanes.
		// @property Random
		.addData("Random", &PlayscreenParameters::Random)
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
		.addData("GaugeType", &PlayscreenParameters::GaugeType)
		/// Request a specific game type.
		// @enumproperty SystemType
		// @param 0 Auto
		// @param 1 BMS
		// @param 2 osu!mania
		// @param 3 o2jam
		// @param 4 Stepmania
		// @param 5 Raindrop
		// @param 6 RDAC
		.addData("SystemType", &PlayscreenParameters::SystemType)
		/// Whether to treat input desired speed as a green number
		// @property GreenNumber
		.addData("GreenNumber", &PlayscreenParameters::GreenNumber)
		/// Whether to enable extended W0 judge
		// @property UseW0
		.addData("UseW0", &PlayscreenParameters::UseW0)
        .addData("SpeedType", &PlayscreenParameters::SpeedType)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// Global game status. Available in most places.
		/// @type GameState
		.beginClass <GameState>("GameState")
		/// Returns currently selected song.
		// @function GetSelectedSong
		// @return The currently selected song.
		.addFunction("GetSelectedSong", &GameState::GetSelectedSong)
		/// Returns the player's current difficulty.
		// @function GetDifficulty
		// @param pn The player number.
		// @return The player's Difficulty7K.
		.addFunction("GetDifficulty", &GameState::GetDifficulty)
		/// Returns the player's current scorekeeper.
		// @function GetScorekeeper7K
		// @param pn The player number.
		// @return The player's @{ScoreKeeper7K}.
		.addFunction("GetScorekeeper7K", &GameState::GetScorekeeper7K)
		/// Returns the player's currently requested parameters.
		// @function GetParameters
		// @param pn The player number.
		// @return The player's PlayscreenParameters.
		.addFunction("GetParameters", &GameState::GetParameters)
		/// Returns the player's currently effective Gauge Type.
		// @function GetCurrentGaugeType
		// @param pn The player number.
		// @return A player's current gauge type.
		.addFunction("GetCurrentGaugeType", &GameState::GetCurrentGaugeType)
		/// Returns the player's currently effective score type.
		// @function GetCurrentScoreType
		// @param pn The player number.
		// @return A player's current score type.
		.addFunction("GetCurrentScoreType", &GameState::GetCurrentScoreType)
		/// Returns the player's currently effective system type.
		// @function GetCurrentSystemType
		// @param pn The player number.
		// @return A player's current system type.
		.addFunction("GetCurrentSystemType", &GameState::GetCurrentSystemType)
		/// Sort the wheel using a criteria.
		// @function SortWheelBy
		// @param crit A criteria. Currently an enum.
		.addFunction("SortWheelBy", &GameState::SortWheelBy)
		/// Starts a new screen. Use "custom:filename" to use your own script.
		// filename is a path relative to the program's working directory.
		// You can use "songselect" as well.
		// @function StartScreen
		// @param screen The string describing the screen to transition to.
		.addFunction("StartScreen", &GameState::StartScreenTransition)
		/// Pops the current screen and goes back up one level.
		// @function ExitScreen
		.addFunction("ExitScreen", &GameState::ExitCurrentScreen)
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