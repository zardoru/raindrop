#include "pch.h"

#include "PlayerContext.h"
#include "GameState.h"

#include "GameWindow.h"
#include "Song.h"
#include "SongDatabase.h"
#include "SongWheel.h"

#include "Screen.h"
#include "ScreenCustom.h"
#include "ScreenSelectMusic.h"

#include "ImageLoader.h"
#include "Song7K.h"

#include "NoteLoader7K.h"
#include "ScoreKeeper7K.h"

namespace Game {
	namespace VSRG {
		SkinMetric PLAYFIELD_SIZE("PlayfieldSize");
		SkinMetric UNITS_PER_MEASURE("UnitsPerMeasure");
	}
}

std::string DirectoryPrefix("GameData/");
#define SkinsPrefix std::string("Skins/")
#define SongsPrefix std::string("Songs")
#define ScriptsPrefix std::string("Scripts/")

using namespace Game;

void GameState::SetSystemFolder(const std::string folder)
{
	DirectoryPrefix = folder;
}

bool GameState::FileExistsOnSkin(const char* Filename, const char* Skin)
{
    std::filesystem::path path(Skin);
    path = DirectoryPrefix / (SkinsPrefix / path / Filename);

    return std::filesystem::exists(path);
}

GameState::GameState(): 
	StageImage(nullptr), 
	SongBG(nullptr)
{
    CurrentSkin = "default";
    SelectedSong = nullptr;
    Database = nullptr;

    // TODO: circular references are possible :(
    std::filesystem::path SkinsDir(DirectoryPrefix + SkinsPrefix);
    std::vector<std::filesystem::path> listing = Utility::GetFileListing(SkinsDir);;
    for (auto s : listing)
    {
		auto st = s.filename().string();
        std::ifstream fallback;
        fallback.open((s / "fallback.txt").string());
        if (fallback.is_open() && s != "default")
        {
            std::string ln;
			while (getline(fallback, ln)) {
				if (Utility::ToLower(ln) != Utility::ToLower(st))
					Fallback[st].push_back(ln);
			}
        }
        if (!Fallback[st].size()) Fallback[st].push_back("default");
    }

	// push the default player
	PlayerInfo.resize(1);
	auto& player = PlayerInfo.back();
	player.profile.Load("machine");
}

std::filesystem::path GameState::GetSkinScriptFile(const char* Filename, const std::string& skin)
{
    std::string Fn = Filename;

    if (Fn.find(".lua") == std::string::npos)
        Fn += ".lua";

    return GetSkinFile(Fn, skin).replace_extension("");
}

std::shared_ptr<Game::Song> GameState::GetSelectedSongShared() const
{
	if (Game::SongWheel::GetInstance().GetSelectedSong())
		return Game::SongWheel::GetInstance().GetSelectedSong();
	else
		return SelectedSong;
}

std::string GameState::GetFirstFallbackSkin()
{
    return Fallback[GetSkin()][0];
}

GameState& GameState::GetInstance()
{
    static GameState* StateInstance = new GameState;
    return *StateInstance;
}

void GameState::SetSelectedSong(std::shared_ptr<VSRG::Song> sng)
{
	SelectedSong = sng;
}

VSRG::Song *GameState::GetSelectedSong() const
{
	auto p = SongWheel::GetInstance().GetSelectedSong().get();
    return p ? p : SelectedSong.get();
}

void Game::GameState::StartScreenTransition(std::string target)
{
	if (target.find("custom") == 0) {
		auto res = Utility::TokenSplit(target, ":");
		if (res.size() == 2)
		{
			auto scr = std::make_shared<ScreenCustom>(res[1]);
			RootScreen->GetTop()->StartTransition(scr);
		}
	}
	else if (target == "songselect") {
		auto scr = std::make_shared<ScreenSelectMusic>();
		scr->Init();
		RootScreen->GetTop()->StartTransition(scr);
	}
}

void Game::GameState::ExitCurrentScreen()
{
	RootScreen->GetTop()->Close();
}

std::filesystem::path GameState::GetSkinFile(const std::string &Name, const std::string &Skin)
{
    std::string Test = GetSkinPrefix(Skin) + Name;

    if (std::filesystem::exists(Test))
        return Test;

    if (Fallback.find(Skin) != Fallback.end())
    {
        for (auto &s : Fallback[Skin])
        {
            if (FileExistsOnSkin(Name.c_str(), s.c_str()))
                return GetSkinFile(Name, s);
        }
    }

    return Test;
}

std::filesystem::path GameState::GetSkinFile(const std::string& Name)
{
    return GetSkinFile(Name, GetSkin());
}

void GameState::Initialize()
{
    if (!Database)
    {
        Database = new SongDatabase("rd.db");

        SongBG = new Texture();
        StageImage = new Texture();
    }
}

GameWindow* GameState::GetWindow()
{
    return &WindowFrame;
}

std::string GameState::GetDirectoryPrefix()
{
    return DirectoryPrefix;
}

std::string GameState::GetSkinPrefix()
{
    // I wonder if a directory transversal is possible. Or useful, for that matter.
    return GetSkinPrefix(GetSkin());
}

std::string GameState::GetSkinPrefix(const std::string& skin)
{
    return DirectoryPrefix + SkinsPrefix + skin + "/";
}

void GameState::SetSkin(std::string Skin)
{
    CurrentSkin = Skin;
}

std::string GameState::GetScriptsDirectory()
{
    return DirectoryPrefix + ScriptsPrefix;
}

SongDatabase* GameState::GetSongDatabase()
{
    return Database;
}

std::filesystem::path GameState::GetFallbackSkinFile(const std::string &Name)
{
    std::string Skin = GetSkin();

    if (Fallback.find(Skin) != Fallback.end())
    {
        for (auto s : Fallback[Skin])
            if (FileExistsOnSkin(Name.c_str(), s.c_str()))
                return GetSkinFile(Name, s);
    }

    return GetSkinPrefix() + Name;
}

bool GameState::PlayerNumberInBounds(int pn) const
{
	return pn >= 0 && pn < PlayerInfo.size();
}

void Game::GameState::SetPlayerContext(VSRG::PlayerContext * pc, int pn)
{
	if (PlayerNumberInBounds(pn)) {
		PlayerInfo[pn].ctx = pc;
	}
}

int GameState::GetCurrentGaugeType(int pn) const
{
	if (PlayerNumberInBounds(pn))
		return PlayerInfo[pn].ctx ? PlayerInfo[pn].ctx->GetCurrentGaugeType() : PlayerInfo[pn].play_parameters.GaugeType;
	return 0;
}

Texture* GameState::GetSongBG()
{
	if (SelectedSong)
	{
		auto toLoad = SelectedSong->SongDirectory / SelectedSong->BackgroundFilename;

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

Texture* GameState::GetSongStage()
{
	auto sng = SelectedSong ? SelectedSong.get() : GetSelectedSong();
	if (sng)
	{
		if (PlayerInfo[0].active_difficulty)
		{
			auto diff = PlayerInfo[0].active_difficulty;
			std::filesystem::path File = Database->GetStageFile(diff->ID);

			// Oh so it's loaded and it's not in the database, fine.
			if (File.string().length() == 0 && diff->Data)
				File = diff->Data->StageFile;

			auto toLoad = sng->SongDirectory / File;

			// ojn files use their cover inside the very ojn
			if (File.extension() == ".ojn")
			{
				size_t read;
				const unsigned char* buf = reinterpret_cast<const unsigned char*>(LoadOJNCover(toLoad, read));
				ImageData data = ImageLoader::GetDataForImageFromMemory(buf, read);
				StageImage->SetTextureData2D(data, true);
				delete[] buf;

				return StageImage;
			}

			if (File.string().length() && std::filesystem::exists(toLoad))
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

Texture* GameState::GetSkinImage(const std::string& Path)
{
    /* Special paths */
    if (Path == "STAGEFILE")
	    return GetSongStage();

    if (Path == "SONGBG")
	    return GetSongBG();

    /* Regular paths */
    if (Path.length())
        return ImageLoader::Load(GetSkinFile(Path, GetSkin()));

	// no path?
    return nullptr;
}

bool GameState::SkinSupportsChannelCount(int Count)
{
    char nstr[256];
    sprintf(nstr, "Channels%d", Count);
    return Configuration::ListExists(nstr);
}

std::string GameState::GetSkin()
{
    return CurrentSkin;
}

Game::VSRG::ScoreKeeper* GameState::GetScorekeeper7K(int pn)
{
	if (PlayerNumberInBounds(pn))
		return PlayerInfo[pn].scorekeeper.get();
	else return nullptr;
}

void GameState::SetScorekeeper7K(std::shared_ptr<Game::VSRG::ScoreKeeper> Other, int pn)
{
    if (PlayerNumberInBounds(pn))
		PlayerInfo[pn].scorekeeper = Other;
}


int GameState::GetCurrentScoreType(int pn) const
{
	if (PlayerNumberInBounds(pn))
		return PlayerInfo[pn].play_parameters.GetScoringType();
	else
		return 0;
}

int GameState::GetCurrentSystemType(int pn) const
{
	if (PlayerNumberInBounds(pn))
		return PlayerInfo[pn].ctx ? PlayerInfo[pn].ctx->GetCurrentSystemType() : PlayerInfo[pn].play_parameters.SystemType;
	else
		return 0;
}

void Game::GameState::SetDifficulty(std::shared_ptr<Game::VSRG::Difficulty> df, int pn)
{
	if (PlayerNumberInBounds(pn)) {
		PlayerInfo[pn].active_difficulty = df;
	}
}

int Game::GameState::GetPlayerCount() const
{
	return PlayerInfo.size();
}

void GameState::SubmitScore(int pn)
{
	if (!PlayerNumberInBounds(pn))
		return;

	auto *player = &PlayerInfo[pn];
	auto d = GetDifficultyShared(pn);
	auto replay = player->ctx->GetReplay();

	if (replay.GetEffectiveParameters().Auto)
		return;

	auto scorekeeper = *player->scorekeeper;
	auto drift = player->ctx->GetDrift();
	auto joffset = player->ctx->GetJudgeOffset();
	auto song = *GetSelectedSong();

	auto submitfunc = [=]() {
		player->profile.Scores.AddScore(
			replay.GetSongHash(),
			replay.GetDifficultyIndex(),
			replay.GetEffectiveParameters(),
			scorekeeper,
			drift,
			joffset
		);

		player->profile.SaveReplay(&song, replay);
	};

	std::async(submitfunc);
}

bool Game::GameState::IsSongUnlocked(Game::Song * song)
{
	return true;
}

void Game::GameState::UnlockSong(Game::Song * song)
{
}

void Game::GameState::SetRootScreen(std::shared_ptr<Screen> root)
{
	RootScreen = root;
}

std::shared_ptr<Screen> Game::GameState::GetCurrentScreen()
{
	return std::shared_ptr<Screen>();
}

std::shared_ptr<Screen> Game::GameState::GetNextScreen()
{
	return std::shared_ptr<Screen>();
}

void GameState::SortWheelBy(int criteria)
{
	SongWheel::GetInstance().SortBy(ESortCriteria(criteria));
}
