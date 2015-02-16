#include "GameGlobal.h"
#include <map>

#include "LuaManager.h"
#include "LuaBridge.h"

#include "GameState.h"

#include "Song.h"
#include "Song7K.h"
#include "SongDC.h"
#include "SongDatabase.h"

#include "ScoreKeeper7K.h"

enum OBJTYPE {
	OB_HOLDS,
	OB_NOTES,
	OB_OBJ,
	OB_SCOBJ
};

struct songHelper
{
	template <class T>
	static uint32 getDifficultyCountForSong(T const *Sng)
	{
		return Sng->Difficulties.size();
	}

	template <class T>
	static void setDifficultyCountForSong(T *Sng, uint32 v)
	{
		return;
	}

	template <class T>
	static GString getDifficultyAuthor(T const *Diff)
	{
		return GameState::GetInstance().GetSongDatabase()->GetArtistForDifficulty(Diff->ID);
	}

	template <class T>
	static void setDifficultyAuthor(T *Diff, GString s)
	{
		return;
	}

	template <class T, class Q>
	static Q* getDifficulty(T *Sng, uint32 idx)
	{
		if (idx < 0 || idx >= getDifficultyCountForSong(Sng))
			return NULL;

		Q* diff = (Q*)Sng->Difficulties[idx];
		return diff;
	}
};

VSRG::Song* toSong7K(Game::Song* Sng)
{
	if (Sng && Sng->Mode == MODE_VSRG)
		return (VSRG::Song*) Sng;
	else
		return NULL;
}

dotcur::Song* toSongDC(Game::Song* Sng)
{
	if (Sng && Sng->Mode == MODE_DOTCUR)
		return (dotcur::Song*) Sng;
	else
		return NULL;
}

GameParameters* GameState::GetParameters()
{
	return &Params;
}

void GameState::InitializeLua(lua_State *L)
{
	SetupScorekeeper7KLuaInterface(L);

	luabridge::getGlobalNamespace(L)
		.beginClass <Game::Song>("Song")
		.addData("Title", &Game::Song::SongName, false)
		.addData("Author", &Game::Song::SongAuthor, false)
		.addData("ID", &Game::Song::ID, false)
		.addData("Mode", &Game::Song::Mode, false)
		.endClass();

	luabridge::getGlobalNamespace(L)
		.beginClass <Game::Song::Difficulty>("Difficulty")
		.addData("Duration", &Game::Song::Difficulty::Duration, false)
		.addData("Name", &Game::Song::Difficulty::Name, false)
		.addData("Offset", &Game::Song::Difficulty::Offset, false)
		.addData("Holds", &Game::Song::Difficulty::TotalHolds, false)
		.addData("Notes", &Game::Song::Difficulty::TotalNotes, false)
		.addData("Objects", &Game::Song::Difficulty::TotalObjects, false)
		.addData("ScoreObjects", &Game::Song::Difficulty::TotalScoringObjects, false)
		.addProperty("Author", &songHelper::getDifficultyAuthor<Game::Song::Difficulty>, 
			&songHelper::setDifficultyAuthor <Game::Song::Difficulty>)
		.endClass();

	luabridge::getGlobalNamespace(L)
		.deriveClass <VSRG::Difficulty, Game::Song::Difficulty>("Difficulty7K")
		.addData("Level", &VSRG::Difficulty::Level, false)
		.addData("Channels", &VSRG::Difficulty::Channels, false)
		.endClass();

	luabridge::getGlobalNamespace(L)
		.deriveClass <dotcur::Difficulty, Game::Song::Difficulty> ("DifficultyDC")
		.endClass();

	luabridge::getGlobalNamespace(L)
		.deriveClass <VSRG::Song, Game::Song>("Song7K")
		.addProperty("DifficultyCount", &songHelper::getDifficultyCountForSong<VSRG::Song>,
			&songHelper::setDifficultyCountForSong<VSRG::Song>)
		.addFunction("GetDifficulty", &VSRG::Song::GetDifficulty)
		.endClass();

	luabridge::getGlobalNamespace(L)
		.deriveClass <dotcur::Song, Game::Song>("SongDC")
		.addProperty("DifficultyCount", &songHelper::getDifficultyCountForSong<dotcur::Song>,
			&songHelper::setDifficultyCountForSong<dotcur::Song>)
		// .addFunction("GetDifficulty", &songHelper::getDifficulty<dotcur::Song, dotcur::Difficulty>)
		.endClass();

	luabridge::getGlobalNamespace(L)
		.beginClass <GameParameters>("GameParameters")
		.addData("Upscroll", &GameParameters::Upscroll)
		.addData("Wave", &GameParameters::Wave)
		.addData("NoFail", &GameParameters::NoFail)
		.addData("Autoplay", &GameParameters::Auto)
		.addData("HiddenMode", &GameParameters::HiddenMode)
		.addData("Rate", &GameParameters::Rate)
		.endClass();

	luabridge::getGlobalNamespace(L)
		.beginClass <GameState> ("GameState")
		.addFunction("GetSelectedSong", &GameState::GetSelectedSong)
		.addProperty("DifficultyIndex", &GameState::GetDifficultyIndex, &GameState::SetDifficultyIndex)
		.addFunction("GetScorekeeper7K", &GameState::GetScorekeeper7K)
		.addFunction("GetParameters", &GameState::GetParameters)
		.endClass()
		.addFunction("toSong7K", toSong7K)
		.addFunction("toSongDC", toSongDC);

	luabridge::push(L, this);
	lua_setglobal(L, "Global");
}