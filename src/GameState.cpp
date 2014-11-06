#include <stdarg.h>
#include <stdio.h>
#include <fstream>

#include "GameGlobal.h"
#include "GameState.h"
#include "GameWindow.h"
#include "Song.h"
#include "SongDatabase.h"

#include "ImageLoader.h"

#define DirectoryPrefix GString("GameData/")
#define SkinsPrefix GString("Skins/")
#define SongsPrefix GString("Songs")
#define ScriptsPrefix GString("Scripts/")

using namespace Game;

GameState::GameState()
{
	CurrentSkin = "default";
	SelectedSong = NULL;
	DifficultyIndex = 0;
}

GameState& GameState::GetInstance()
{
	static GameState* StateInstance = new GameState;
	return *StateInstance;
}

Game::Song *GameState::GetSelectedSong()
{
	return SelectedSong;
}

void GameState::SetSelectedSong(Game::Song* Song)
{
	SelectedSong = Song;
}

void GameState::Initialize()
{
	Database = new SongDatabase("rd.db");
}

void GameState::SetDifficultyIndex(uint32 Index)
{
	DifficultyIndex = Index;
}

uint32 GameState::GetDifficultyIndex() const
{
	return DifficultyIndex;
}

GameWindow* GameState::GetWindow()
{
	return &WindowFrame;
}

GString GameState::GetDirectoryPrefix()
{
	return DirectoryPrefix;
}

GString GameState::GetSkinPrefix()
{
	// I wonder if a directory transversal is possible. Or useful, for that matter.
	return DirectoryPrefix + SkinsPrefix + CurrentSkin + "/";
}

void GameState::SetSkin(GString Skin)
{
	CurrentSkin = Skin;
}

GString GameState::GetScriptsDirectory()
{
	return DirectoryPrefix + ScriptsPrefix;
}

SongDatabase* GameState::GetSongDatabase()
{
	return Database;
}

Image* GameState::GetSkinImage(Directory Path)
{
	return ImageLoader::Load(GetSkinPrefix() / Path);
}

bool GameState::SkinSupportsChannelCount(int Count)
{
	char nstr[256];
	sprintf(nstr, "Channels%d", Count);
	return Configuration::ListExists(nstr);
}