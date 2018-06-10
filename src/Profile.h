#pragma once

#include "ScoreDatabase.h"

namespace Game {
	
	class Profile {
		void AssureProfilePathExistence();
	public:
		std::string Name;
		ScoreDatabase Scores;

		std::filesystem::path GetPath();

		// based off names
		bool Load(std::string Name);
		bool Save();

		void SaveReplay(
			const VSRG::Song* song, 
			const VSRG::Replay &replay
		);

		ScoreRow GetDifficultyScore(VSRG::Difficulty* diff);

		static std::vector<std::string> GetProfileList();
	};
}