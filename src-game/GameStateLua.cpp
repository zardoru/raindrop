#include "pch.h"

#include "LuaManager.h"
#include "LuaBridge.h"

#include "GameState.h"

#include "Song.h"
#include "Song7K.h"
#include "SongDatabase.h"
#include "SongWheel.h"

#include "ScoreKeeper7K.h"

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

Game::VSRG::PlayscreenParameters* GameState::GetParameters(int pn)
{
	if (PlayerNumberInBounds(pn))
		return &PlayerInfo[pn].play_parameters;
	else return nullptr;
}

Game::VSRG::Difficulty * Game::GameState::GetDifficulty(int pn)
{
	if(GetSelectedSong())
		return ((Game::VSRG::Song*)GetSelectedSong())->GetDifficulty(SongWheel::GetInstance().GetDifficulty());
	return nullptr;
}

std::shared_ptr<Game::VSRG::Difficulty> Game::GameState::GetDifficultyShared(int pn)
{
	if (PlayerNumberInBounds(pn) && PlayerInfo[pn].active_difficulty)
		return PlayerInfo[pn].active_difficulty;
	else {
		return ((Game::VSRG::Song*)GetSelectedSong())->Difficulties[SongWheel::GetInstance().GetDifficulty()];
	}

	return nullptr;
}


int DoGameScript(lua_State *S)
{
    LuaManager* Lua = GetObjectFromState<LuaManager>(S, "Luaman");
    std::string File = luaL_checkstring(S, 1);
    Lua->Require(GameState::GetInstance().GetScriptsDirectory() + File);
    return 1;
}

/// Game Data types. Generally inert.
// @engineclass GameState
void GameState::InitializeLua(lua_State *L)
{
	LuaManager l(L);
	l.Register(DoGameScript, "game_require");

	Game::VSRG::SetupScorekeeperLuaInterface(L);

	luabridge::getGlobalNamespace(L)
		/// Base Song class.
		/// @type Song
		.beginClass <Game::Song>("Song")
		/// The song's title.
		// @roproperty Title 
		.addData("Title", &Game::Song::Title, false)
		/// The song's author.
		// @roproperty Author 
		.addData("Author", &Game::Song::Artist, false)
		/// The song's subtitle.
		// @roproperty Subtitle 
		.addData("Subtitle", &Game::Song::Subtitle, false)
		/// The song's database ID.
		// @roproperty ID 
		.addData("ID", &Game::Song::ID, false)
		/// The song's mode. Currently only 1.
		// @roproperty Mode 
		.addData("Mode", &Game::Song::Mode, false)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// The Base Difficulty class.
		/// @type Difficulty
		.beginClass <Game::Song::Difficulty>("Difficulty")
		/// Duration of the difficulty, in seconds.
		// @roproperty Duration
		.addData("Duration", &Game::Song::Difficulty::Duration, false)
		/// Difficulty name.
		// @roproperty Name
		.addData("Name", &Game::Song::Difficulty::Name, false)
		/// Offset.
		// @roproperty Offset
		.addData("Offset", &Game::Song::Difficulty::Offset, false)
		/// Object count.
		// @roproperty Objects
		.addProperty("Objects", 
			&songHelper::GetObjCount<Game::Song::Difficulty>,
			&songHelper::SetObjCount<Game::Song::Difficulty>)
		/// Count of objects that matter for score.
		// @roproperty ScoreObjects
		.addProperty("ScoreObjects",
			&songHelper::GetScoreObjCount<Game::Song::Difficulty>,
			&songHelper::SetScoreObjCount<Game::Song::Difficulty>)
		/// Difficulty's Author.
		// @roproperty Author
		.addProperty("Author", 
			&songHelper::getDifficultyAuthor<Game::Song::Difficulty>, 
			&songHelper::setDifficultyAuthor <Game::Song::Difficulty>)
		/// Difficulty's Genre.
		// @roproperty Genre
		.addProperty("Genre", 
			&songHelper::getDifficultyGenre<Game::Song::Difficulty>, 
			&songHelper::setDifficultyGenre<Game::Song::Difficulty>)
		.endClass();

	luabridge::getGlobalNamespace(L)
		/// VSRG specific difficulty class.
		/// @type Difficulty7K
		.deriveClass <VSRG::Difficulty, Game::Song::Difficulty>("Difficulty7K")
		/// Level, as informed by the loader.
		// @roproperty Level
		.addProperty("Level", 
			&songHelper::getLevel<VSRG::Difficulty>,
			&songHelper::setLevel<VSRG::Difficulty>)
		/// Effective channels in use.
		// @roproperty Channels
		.addData("Channels", &VSRG::Difficulty::Channels, false)
		.endClass();


	luabridge::getGlobalNamespace(L)
		/// Song class, specific for VSRG.
		/// @type Song7K
		.deriveClass <VSRG::Song, Game::Song>("Song7K")
		/// Difficulty count
		// @roproperty DifficultyCount
		.addProperty("DifficultyCount", &songHelper::getDifficultyCountForSong<VSRG::Song>,
			&songHelper::setDifficultyCountForSong<VSRG::Song>)
		/// Get a difficulty, by index. Can return nil.
		// @function GetDifficulty
		// @param index The difficulty index.
		// @return A Difficulty7K. Can be nil.
		.addFunction("GetDifficulty", &VSRG::Song::GetDifficulty)
		.endClass();

	using namespace Game::VSRG;
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