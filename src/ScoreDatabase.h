#pragma once


namespace Game {

	struct ScoreRow {
		long long rank_score;
		int exscore;
		int score;
		VSRG::ScoreType score_type;
		float gauge;
		VSRG::LifeType gauge_type;
		int hits;
		int max_combo;
		int judgments[6];
		int misses;
		float avghit;
		float stdev;
		double offset;
		double judgeoffset;

		ScoreRow() {
			exscore = score = 0;
			gauge = 0;
			avghit = stdev = 0;
			rank_score = 0;
			score_type = VSRG::ST_SCORE;
			gauge_type = VSRG::LT_AUTO;
			memset(judgments, 0, sizeof(judgments));
			hits = misses = 0;
			offset = judgeoffset = 0;
		}

		// returns true if the score is to be considered 
		// anything that was actually played.
		bool IsPlayedScore() {
			// enough things are zero to say we didn't play this thing
			if (hits == misses && hits == 0) {
				if (score == 0 || score_type == VSRG::LT_AUTO)
					return false;
			}

			return true;
		}
	};

	class ScoreDatabase {
		sqlite3* db;
		sqlite3_stmt *stAddScore,
			*stBestType,
			*stBestEx,
			*stBestRank,
			*stAllScores;
		
		void CleanupStatements();
	public:
		ScoreDatabase();
		~ScoreDatabase();
		void Open(std::filesystem::path path);
		void AddScore(
			std::string charthash,
			int diffindex,
			VSRG::ScoreType type,
			VSRG::LifeType gaugetype,
			VSRG::ScoreKeeper keeper, 
			double offset, // note time displacement
			double judgeoffset); // judgment time displacement

		ScoreRow GetBestScoreForSong(std::string songhash, int diffindex, VSRG::ScoreType st);
		ScoreRow GetBestScoreForSongEX(std::string songhash, int diffindex);
		ScoreRow GetBestScoreForSongRank(std::string songhash, int diffindex);

		std::vector<ScoreRow> GetAllScoresForSong(std::string songhash, int diffindex);
	};
}