#include <stdarg.h>
#include <stdio.h>
#include <fstream>

#include "GameGlobal.h"
#include "GameState.h"
#include "GameWindow.h"
#include "Song.h"
#include "SongDatabase.h"
#include "SongWheel.h"

#include "ImageLoader.h"
#include "Song7K.h"

#include "NoteLoader7K.h"

#define DirectoryPrefix GString("GameData/")
#define SkinsPrefix GString("Skins/")
#define SongsPrefix GString("Songs")
#define ScriptsPrefix GString("Scripts/")

using namespace Game;

GameState::GameState()
{
	CurrentSkin = "default";
	SelectedSong = NULL;
	SKeeper7K = NULL;
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

GString GameState::GetSkinFile(Directory Name)
{
	GString Orig = GetSkinPrefix() + Name.path();

	if (Utility::FileExists(Orig))
	{
		return Orig;
	}
	else
	{
		return GetFallbackSkinFile(Name);
	}
}

void GameState::Initialize()
{
	Database = new SongDatabase("rd.db");

	SongBG = new Image();
	StageImage = new Image();
}

void GameState::SetDifficultyIndex(uint32 Index)
{
	SongWheel::GetInstance().SetDifficulty(Index);
}

uint32 GameState::GetDifficultyIndex() const
{
	return SongWheel::GetInstance().GetDifficulty();
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

GString GameState::GetFallbackSkinFile(Directory Name)
{
	GString SkinFallback = Configuration::GetSkinConfigs("Fallback");

	// Actually, how many levels of fallback recursion should we allow?
	if (SkinFallback.length() == 0)
		SkinFallback = "default";
	SkinFallback += "/";

	return DirectoryPrefix + SkinsPrefix + SkinFallback + Name.path();
}

Image* GameState::GetSkinImage(Directory Path)
{
	/* Special paths */
	if (Path.path() == "STAGEFILE")
	{
		if (SelectedSong)
		{
			if (SelectedSong->Mode == MODE_VSRG)
			{
				VSRG::Song *Song = static_cast<VSRG::Song*>(SelectedSong);

				if (Song->Difficulties.size() > GetDifficultyIndex())
				{
					GString File = Database->GetStageFile(Song->Difficulties.at(GetDifficultyIndex())->ID);
					Directory toLoad;

					// Oh so it's loaded and it's not in the database, fine.
					if (File.length() == 0 && Song->Difficulties.at(GetDifficultyIndex())->Data)
						File = Song->Difficulties.at(GetDifficultyIndex())->Data->StageFile;

					toLoad = SelectedSong->SongDirectory / File.c_str();

					// ojn files use their cover inside the very ojn
					if (Directory(File).GetExtension() == "ojn")
					{
						size_t read;
						const unsigned char* buf = (const unsigned char*)LoadOJNCover(toLoad, read);
						ImageData data = ImageLoader::GetDataForImageFromMemory(buf, read);
						StageImage->SetTextureData(&data, true);
					}

					if (File.length() && Utility::FileExists(toLoad.path()))
					{
						StageImage->Assign(toLoad, ImageData::SM_DEFAULT, ImageData::WM_DEFAULT, true);
						return StageImage;
					}
					else return NULL;
				}
				else return NULL; // Oh okay, no difficulty assigned.
			}
			else // Stage file not supported for DC songs yet
				return NULL; 
		}
		else return NULL;
	}
	else if (Path.path() == "SONGBG")
	{
		if (SelectedSong)
		{
			Directory toLoad = SelectedSong->SongDirectory / SelectedSong->BackgroundFilename.c_str();

			if (SelectedSong->BackgroundFilename.length() && Utility::FileExists(toLoad.path()))
			{
				SongBG->Assign(toLoad, ImageData::SM_DEFAULT, ImageData::WM_DEFAULT, true);
				return SongBG;
			}
			else return NULL;
		}
		else return NULL;
	}

	/* Regular paths */
	if (Path.path().length())
		return ImageLoader::Load(GetSkinFile(Path));
	else return NULL;
}

bool GameState::SkinSupportsChannelCount(int Count)
{
	char nstr[256];
	sprintf(nstr, "Channels%d", Count);
	return Configuration::ListExists(nstr);
}

ScoreKeeper7K *GameState::GetScorekeeper7K()
{
	return SKeeper7K;
}

void GameState::SetScorekeeper7K(ScoreKeeper7K *Other)
{
	SKeeper7K = Other;
}