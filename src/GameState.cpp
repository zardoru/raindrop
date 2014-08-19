#include <stdarg.h>
#include <fstream>

#include "GameGlobal.h"
#include "GameState.h"
#include "GameWindow.h"
#include "Song.h"
#include "SongDatabase.h"

#include "ImageLoader.h"

#define DirectoryPrefix String("GameData/")
#define SkinsPrefix String("Skins/")
#define SongsPrefix String("Songs")
#define ScriptsPrefix String("Scripts/")

using namespace Game;

GameState::GameState()
{
	CurrentSkin = "default";
}

GameState& GameState::GetInstance()
{
	static GameState* StateInstance = new GameState;
	return *StateInstance;
}

void GameState::Initialize()
{
	Database = new SongDatabase("songdatabase.db");
}

GameWindow* GameState::GetWindow()
{
	return &WindowFrame;
}

String GameState::GetDirectoryPrefix()
{
	return DirectoryPrefix;
}

String GameState::GetSkinPrefix()
{
	// I wonder if a directory transversal is possible. Or useful, for that matter.
	return DirectoryPrefix + SkinsPrefix + CurrentSkin + "/";
}

void GameState::SetSkin(String Skin)
{
	CurrentSkin = Skin;
}

String GameState::GetScriptsDirectory()
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