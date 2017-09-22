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
		return &PlayerInfo[pn].Params;
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
	if (PlayerNumberInBounds(pn) && PlayerInfo[pn].Diff)
		return PlayerInfo[pn].Diff;
	else {
		return ((Game::VSRG::Song*)GetSelectedSong())->Difficulties[SongWheel::GetInstance().GetDifficulty()];
	}

	return nullptr;
}

void GameState::InitializeLua(lua_State *L)
{
	Game::VSRG::SetupScorekeeperLuaInterface(L);

	luabridge::getGlobalNamespace(L)
		.beginClass <Game::Song>("Song")
		.addData("Title", &Game::Song::SongName, false)
		.addData("Author", &Game::Song::SongAuthor, false)
		.addData("Subtitle", &Game::Song::Subtitle, false)
		.addData("ID", &Game::Song::ID, false)
		.addData("Mode", &Game::Song::Mode, false)
		.endClass();

	luabridge::getGlobalNamespace(L)
		.beginClass <Game::Song::Difficulty>("Difficulty")
		.addData("Duration", &Game::Song::Difficulty::Duration, false)
		.addData("Name", &Game::Song::Difficulty::Name, false)
		.addData("Offset", &Game::Song::Difficulty::Offset, false)
		
		.addProperty("Objects", &songHelper::GetObjCount<Game::Song::Difficulty>,
			&songHelper::SetObjCount<Game::Song::Difficulty>)

		.addProperty("ScoreObjects", &songHelper::GetScoreObjCount<Game::Song::Difficulty>,
			&songHelper::SetScoreObjCount<Game::Song::Difficulty>)
		
		.addProperty("Author", &songHelper::getDifficultyAuthor<Game::Song::Difficulty>, 
			&songHelper::setDifficultyAuthor <Game::Song::Difficulty>)
		
		.addProperty("Genre", &songHelper::getDifficultyGenre<Game::Song::Difficulty>, 
			&songHelper::setDifficultyGenre<Game::Song::Difficulty>)
		.endClass();

	luabridge::getGlobalNamespace(L)
		.deriveClass <VSRG::Difficulty, Game::Song::Difficulty>("Difficulty7K")
		.addData("Level", &VSRG::Difficulty::Level, false)
		.addData("Channels", &VSRG::Difficulty::Channels, false)
		.endClass();


	luabridge::getGlobalNamespace(L)
		.deriveClass <VSRG::Song, Game::Song>("Song7K")
		.addProperty("DifficultyCount", &songHelper::getDifficultyCountForSong<VSRG::Song>,
			&songHelper::setDifficultyCountForSong<VSRG::Song>)
		.addFunction("GetDifficulty", &VSRG::Song::GetDifficulty)
		.endClass();

	using namespace Game::VSRG;
	luabridge::getGlobalNamespace(L)
		.beginClass <PlayscreenParameters>("PlayscreenParameters")
		.addData("Upscroll", &PlayscreenParameters::Upscroll)
		.addData("Wave", &PlayscreenParameters::Wave)
		.addData("NoFail", &PlayscreenParameters::NoFail)
		.addData("Autoplay", &PlayscreenParameters::Auto)
		.addData("HiddenMode", &PlayscreenParameters::HiddenMode)
		.addData("Rate", &PlayscreenParameters::Rate)
		.addData("Random", &PlayscreenParameters::Random)
		.addData("GaugeType", &PlayscreenParameters::GaugeType)
		.addData("SystemType", &PlayscreenParameters::SystemType)
		.addData("GreenNumber", &PlayscreenParameters::GreenNumber)
		.addData("UseW0", &PlayscreenParameters::UseW0)
		.endClass();

	luabridge::getGlobalNamespace(L)
		.beginClass <GameState>("GameState")
		.addFunction("GetSelectedSong", &GameState::GetSelectedSong)
		.addFunction("GetDifficulty", &GameState::GetDifficulty)
		.addFunction("GetScorekeeper7K", &GameState::GetScorekeeper7K)
		.addFunction("GetParameters", &GameState::GetParameters)
		.addFunction("GetCurrentGaugeType", &GameState::GetCurrentGaugeType)
		.addFunction("GetCurrentScoreType", &GameState::GetCurrentScoreType)
		.addFunction("GetCurrentSystemType", &GameState::GetCurrentSystemType)
		.addFunction("SortWheelBy", &GameState::SortWheelBy)
		.addFunction("StartScreen", &GameState::StartScreenTransition)
		.addFunction("ExitScreen", &GameState::ExitCurrentScreen)
		.endClass();

	luabridge::push(L, this);
	lua_setglobal(L, "Global");

	luabridge::getGlobalNamespace(L)
		.beginNamespace("System")
		.addFunction("SetConfig", Configuration::SetConfig)
		.addFunction("ReadConfigF", Configuration::GetConfigf)
		.addFunction("ReadConfigS", Configuration::GetConfigs)
		.addFunction("ReloadConfiguration", Configuration::Reload)
		.endNamespace();
}