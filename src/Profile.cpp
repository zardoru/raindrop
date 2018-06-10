#include "pch.h"

#include "Song7K.h"
#include "Replay7K.h"
#include "Profile.h"


const std::filesystem::path PROFILE_DIR = "./profiles";

const std::filesystem::path SCOREDB_FILENAME = "scores.db";

const std::filesystem::path REPLAYS_DIR = "replays";

namespace Game {
	void Profile::AssureProfilePathExistence()
	{
		auto path = GetPath();
		std::filesystem::create_directories(path);

		auto replaypath = path / REPLAYS_DIR;
		std::filesystem::create_directory(replaypath);
	}

	std::filesystem::path Profile::GetPath()
	{
		return std::filesystem::absolute(PROFILE_DIR) / Name;
	}

	bool Profile::Load(std::string Name)
	{
		this->Name = Name;
		AssureProfilePathExistence();

		auto path = GetPath();
		Scores.Open(path / SCOREDB_FILENAME);

		return true;
	}

	bool Profile::Save()
	{
		AssureProfilePathExistence();


		return false;
	}

	void Profile::SaveReplay(const VSRG::Song* song, const VSRG::Replay & replay)
	{
		time_t now;
		time(&now);
		auto tm = localtime(&now);

		// format current datetime
		char date_str[512];
		strftime(date_str, 512, "%F %H.%M", tm);

		char replay_filename[512];

		// format that filename
		snprintf(
			replay_filename, 
			512,
			"[%s] %s - %s.rdr", 
			date_str, 
			song->Artist.c_str(), 
			song->Title.c_str()
		);

		// put it out
		replay.Save(GetPath() / REPLAYS_DIR / replay_filename);
	}

	ScoreRow Profile::GetDifficultyScore(VSRG::Difficulty * diff)
	{
		return ScoreRow();
	}

	std::vector<std::string> Profile::GetProfileList()
	{
		return std::vector<std::string>();
	}
}