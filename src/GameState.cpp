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

bool GameState::FileExistsOnSkin(const char* Filename, const char* Skin)
{
	Directory path(Skin);
	path = DirectoryPrefix / (SkinsPrefix / path / Filename);

	return Utility::FileExists(path);
}

GameState::GameState()
{
	CurrentSkin = "default";
	SelectedSong = nullptr;
	SKeeper7K = nullptr;
	Database = nullptr;
	Params = make_shared<GameParameters>();

	// TODO: circular references are possible :(
	Directory SkinsDir(DirectoryPrefix + SkinsPrefix);
	vector<GString> listing;
	SkinsDir.ListDirectory(listing, Directory::FS_DIR);
	for (auto s : listing)
	{
		std::ifstream fallback;
		fallback.open((SkinsDir / s.c_str() / "fallback").c_path());
		if (fallback.is_open() && s != "default")
		{
			GString ln;
			while (getline(fallback, ln))
				if (Utility::ToLower(ln) != Utility::ToLower(s))
				{
					Directory dir(ln); dir.Normalize(true);
					Fallback[s].push_back(dir);
				}
		}
		if (!Fallback[s].size()) Fallback[s].push_back("default");

	}
}

GString GameState::GetSkinScriptFile(const char* Filename, const GString& skin)
{
	GString Fn = Filename;

	if (Fn.find(".lua") == GString::npos)
		Fn += ".lua";

	// Since dots are interpreted as "look into this directory", we want to eliminate the extra .lua for require purposes.
	return Utility::RemoveExtension(GetSkinFile(Fn, skin));
}

shared_ptr<Game::Song> GameState::GetSelectedSongShared()
{
	return SelectedSong;
}

GString GameState::GetFirstFallbackSkin()
{
	return Fallback[GetSkin()][0];
}

GameState& GameState::GetInstance()
{
	static GameState* StateInstance = new GameState;
	return *StateInstance;
}

Song *GameState::GetSelectedSong()
{
	return SelectedSong.get();
}

void GameState::SetSelectedSong(shared_ptr<Game::Song> Song)
{
	SelectedSong = Song;
}

GString GameState::GetSkinFile(const GString &Name, const GString &Skin)
{
	GString Test = GetSkinPrefix(Skin) + Name;

	if (Utility::FileExists(Test))
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

GString GameState::GetSkinFile(const GString& Name)
{
	return GetSkinFile(Name, GetSkin());
}

void GameState::Initialize()
{
	if (!Database)
	{
		Database = new SongDatabase("rd.db");

		SongBG = new Image();
		StageImage = new Image();
	}
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
	return GetSkinPrefix(GetSkin());
}

GString GameState::GetSkinPrefix(const GString& skin)
{
	return DirectoryPrefix + SkinsPrefix + skin + "/";
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

GString GameState::GetFallbackSkinFile(const GString &Name)
{
	GString Skin = GetSkin();

	if (Fallback.find(Skin) != Fallback.end())
	{
		for (auto s : Fallback[Skin])
		if (FileExistsOnSkin(Name.c_str(), s.c_str()))
			return GetSkinFile(Name, s);
	}

	return GetSkinPrefix() + Name;
}

void GameState::SetCurrentGaugeType(int GaugeType)
{
	this->CurrentGaugeType = GaugeType;
}

int GameState::GetCurrentGaugeType() const
{
	return CurrentGaugeType;
}

Image* GameState::GetSkinImage(const GString& Path)
{
	/* Special paths */
	if (Path == "STAGEFILE")
	{
		if (SelectedSong)
		{
			if (SelectedSong->Mode == MODE_VSRG)
			{
				VSRG::Song *Song = static_cast<VSRG::Song*>(SelectedSong.get());

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
						const unsigned char* buf = reinterpret_cast<const unsigned char*>(LoadOJNCover(toLoad, read));
						ImageData data = ImageLoader::GetDataForImageFromMemory(buf, read);
						StageImage->SetTextureData(&data, true);
						delete[] buf;

						return StageImage;
					}

					if (File.length() && Utility::FileExists(toLoad.path()))
					{
						StageImage->Assign(toLoad, ImageData::SM_DEFAULT, ImageData::WM_DEFAULT, true);
						return StageImage;
					}
					else return nullptr;
				}
				else return nullptr; // Oh okay, no difficulty assigned.
			}
			else // Stage file not supported for DC songs yet
				return nullptr;
		}
		else return nullptr;
	}
	else if (Path == "SONGBG")
	{
		if (SelectedSong)
		{
			Directory toLoad = SelectedSong->SongDirectory / SelectedSong->BackgroundFilename.c_str();

			if (SelectedSong->BackgroundFilename.length() && Utility::FileExists(toLoad.path()))
			{
				SongBG->Assign(toLoad, ImageData::SM_DEFAULT, ImageData::WM_DEFAULT, true);
				return SongBG;
			}
			else return nullptr;
		}
		else return nullptr;
	}

	/* Regular paths */
	if (Path.length())
		return ImageLoader::Load(GetSkinFile(Path, GetSkin()));
	else return nullptr;
}

bool GameState::SkinSupportsChannelCount(int Count)
{
	char nstr[256];
	sprintf(nstr, "Channels%d", Count);
	return Configuration::ListExists(nstr);
}

GString GameState::GetSkin()
{
	return CurrentSkin;
}

ScoreKeeper7K* GameState::GetScorekeeper7K()
{
	return SKeeper7K.get();
}

void GameState::SetScorekeeper7K(shared_ptr<ScoreKeeper7K> Other)
{
	SKeeper7K = Other;
}