#include "pch.h"

#include "ScoreKeeper7K.h"
#include "ScoreDatabase.h"
#include "Logging.h"

auto qScoreDDL = "CREATE TABLE IF NOT EXISTS [scores](\
	[charthash] varchar(64) NOT NULL,\
	[diffindex] INT NOT NULL DEFAULT 0,\
	[rankpts] int64 NOT NULL DEFAULT 0,\
	[exscore] int NOT NULL DEFAULT 0,\
	[score] int64 NOT NULL DEFAULT 0,\
	[scoretype] int NOT NULL DEFAULT(-1),\
	[gauge] float NOT NULL DEFAULT 1,\
	[gaugetype] int NOT NULL DEFAULT(-1),\
	[hits] int NOT NULL DEFAULT 0,\
	[maxcombo] int NOT NULL DEFAULT 0,\
	[w0] int NOT NULL DEFAULT 0,\
	[w1] int NOT NULL DEFAULT 0,\
	[w2] int NOT NULL DEFAULT 0,\
	[w3] int NOT NULL DEFAULT 0,\
	[w4] int NOT NULL DEFAULT 0,\
	[w5] int NOT NULL DEFAULT 0,\
	[misses] int NOT NULL DEFAULT 0,\
	[avghit] float NOT NULL DEFAULT 0,\
	[stdev] int NOT NULL DEFAULT 0,\
	[offset] float NOT NULL DEFAULT 0,\
	[judgeoffset] float NOT NULL DEFAULT 0,\
	[date] DATETIME NOT NULL DEFAULT (datetime('now','localtime'))\
    );\
	CREATE INDEX IF NOT EXISTS [songid]\
	ON[scores](\
		[charthash],\
		[diffindex]); ";

auto qAddScore = "INSERT INTO scores VALUES(\
	$hash,$diffindex,$rankpts,$ex,$score,$scoretype,$gauge,$gaugetype,\
    $hits,$maxcombo,$w0,$w1,$w2,$w3,$w4,$w5,$misses,$avghit,$stdev,$offset,$judgeoffset,datetime('now'));";

auto qBestEx = "SELECT rankpts, exscore, score, scoretype, gauge, gaugetype, \
			    hits, maxcombo, w0, w1, w2, w3, w4, w5, misses, avghit, stdev, \
				offset, judgeoffset FROM scores WHERE\
				charthash=$hash AND diffindex=$diffindex ORDER BY exscore DESC LIMIT 1";

auto qBestRank = "SELECT rankpts, exscore, score, scoretype, gauge, gaugetype, \
			    hits, maxcombo, w0, w1, w2, w3, w4, w5, misses, avghit, stdev, \
				offset, judgeoffset FROM scores WHERE\
				charthash=$hash AND diffindex=$diffindex ORDER BY rankpts DESC LIMIT 1";

auto qBestType = "SELECT rankpts, exscore, score, scoretype, gauge, gaugetype, \
			    hits, maxcombo, w0, w1, w2, w3, w4, w5, misses, avghit, stdev, \
				offset, judgeoffset FROM scores WHERE\
				charthash=$hash AND diffindex=$diffindex AND scoretype=$type \
				ORDER BY score DESC LIMIT 1";

auto qAllScores = "SELECT rankpts, exscore, score, scoretype, gauge, gaugetype, \
			    hits, maxcombo, w0, w1, w2, w3, w4, w5, misses, avghit, stdev, \
				offset, judgeoffset FROM scores WHERE\
				charthash=$hash AND diffindex=$diffindex\
				ORDER BY rankpts DESC";

#define SC(x) \
{ret=x; if(ret!=SQLITE_OK && ret != SQLITE_DONE) \
{Log::Printf("sqlite: %ls (code %d)\n",Utility::Widen(sqlite3_errmsg(db)).c_str(), ret); Utility::DebugBreak(); }}

namespace Game {

	ScoreDatabase::ScoreDatabase()
	{
		stAddScore = nullptr;
		stAllScores = nullptr;
		stBestEx = nullptr;
		stBestRank = nullptr;
		stBestType = nullptr;

		db = nullptr;
	}

	void ScoreDatabase::CleanupStatements()
	{
		sqlite3_finalize(stAddScore);
		sqlite3_finalize(stAllScores);
		sqlite3_finalize(stBestEx);
		sqlite3_finalize(stBestRank);
		sqlite3_finalize(stBestType);
	}

	ScoreDatabase::~ScoreDatabase() 
	{
		CleanupStatements();

		if (db)
			sqlite3_close(db);
	}

	void ScoreDatabase::Open(std::filesystem::path path)
	{
		if (db) {
			CleanupStatements();
			sqlite3_close(db);
			db = nullptr;
		}

		sqlite3_open_v2(
			path.string().c_str(),
			&db,
			SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			NULL
		);

		char* erra;
		const char* err;
		sqlite3_exec(db, qScoreDDL, NULL, NULL, &erra);
		int ret;

		SC(sqlite3_prepare_v2(db, qAddScore, strlen(qAddScore), &stAddScore, &err));
		sqlite3_prepare_v2(db, qBestEx, strlen(qBestEx), &stBestEx, &err);
		sqlite3_prepare_v2(db, qBestRank, strlen(qBestRank), &stBestRank, &err);
		sqlite3_prepare_v2(db, qBestType, strlen(qBestType), &stBestType, &err);
		sqlite3_prepare_v2(db, qAllScores, strlen(qAllScores), &stAllScores, &err);
	}

	void ScoreDatabase::AddScore(
		std::string charthash,
		int diffindex,
		VSRG::ScoreType type,
		VSRG::LifeType gaugetype,
		VSRG::ScoreKeeper keeper, 
		double offset, 
		double judgeoffset)
	{
		// az: this takes forever to type. 

		sqlite3_bind_text(
			stAddScore, 
			sqlite3_bind_parameter_index(stAddScore, "$hash"),
			charthash.c_str(), 
			charthash.length(), 
			SQLITE_STATIC);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$diffindex"),
			diffindex);

		sqlite3_bind_int64(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$rankpts"),
			keeper.getRankPoints()
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$ex"),
			keeper.getScore(VSRG::ST_EX)
		);

		sqlite3_bind_int64(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$score"),
			keeper.getScore(type)
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$scoretype"),
			type
		);

		sqlite3_bind_double(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$gauge"),
			keeper.getLifebarAmount(gaugetype)
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$gaugetype"),
			gaugetype
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$hits"),
			keeper.getTotalNotes()
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$maxcombo"),
			keeper.getScore(VSRG::ST_MAX_COMBO)
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$w0"),
			keeper.getJudgmentCount(VSRG::SKJ_W0)
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$w1"),
			keeper.getJudgmentCount(VSRG::SKJ_W1)
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$w2"),
			keeper.getJudgmentCount(VSRG::SKJ_W2)
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$w3"),
			keeper.getJudgmentCount(VSRG::SKJ_W3)
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$w4"),
			keeper.getJudgmentCount(VSRG::SKJ_W4)
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$w5"),
			keeper.getJudgmentCount(VSRG::SKJ_W5)
		);

		sqlite3_bind_int(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$misses"),
			keeper.getJudgmentCount(VSRG::SKJ_MISS)
		);

		sqlite3_bind_double(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$avghit"),
			keeper.getAvgHit()
		);

		sqlite3_bind_double(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$stdev"),
			keeper.getHitStDev()
		);

		sqlite3_bind_double(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$offset"),
			offset
		);

		sqlite3_bind_double(
			stAddScore,
			sqlite3_bind_parameter_index(stAddScore, "$judgeoffset"),
			judgeoffset
		);

		int ret;
		SC(sqlite3_step(stAddScore));
		sqlite3_reset(stAddScore);
	}

	void prepareBest(sqlite3_stmt *stmt, std::string &charthash, int diffindex) 
	{
		sqlite3_bind_text(
			stmt,
			sqlite3_bind_parameter_index(stmt, "$hash"),
			charthash.c_str(),
			charthash.length(),
			SQLITE_STATIC
		);

		sqlite3_bind_int(
			stmt,
			sqlite3_bind_parameter_index(stmt, "$diffindex"),
			diffindex
		);
	}

	ScoreRow toScoreRow(sqlite3_stmt *stmt)
	{
		ScoreRow ret;

		// that's what we call, "a pain"
		ret.rank_score = sqlite3_column_int64(stmt, 0);
		ret.exscore = sqlite3_column_int(stmt, 1);
		ret.score = sqlite3_column_int(stmt, 2);
		ret.score_type = (VSRG::ScoreType)sqlite3_column_int(stmt, 3);
		ret.gauge = sqlite3_column_double(stmt, 4);
		ret.gauge_type = (VSRG::LifeType)sqlite3_column_int(stmt, 5);
		ret.hits = sqlite3_column_int(stmt, 6);
		ret.max_combo = sqlite3_column_int(stmt, 7);
		ret.judgments[0] = sqlite3_column_int(stmt, 8);
		ret.judgments[1] = sqlite3_column_int(stmt, 9);
		ret.judgments[2] = sqlite3_column_int(stmt, 10);
		ret.judgments[3] = sqlite3_column_int(stmt, 11);
		ret.judgments[4] = sqlite3_column_int(stmt, 12);
		ret.judgments[5] = sqlite3_column_int(stmt, 13);
		ret.misses = sqlite3_column_int(stmt, 14);
		ret.avghit = sqlite3_column_int(stmt, 15);
		ret.stdev = sqlite3_column_int(stmt, 16);
		ret.offset = sqlite3_column_double(stmt, 17);
		ret.judgeoffset = sqlite3_column_double(stmt, 18);

		return ret;
	}

	ScoreRow ScoreDatabase::GetBestScoreForSong(std::string songhash, int diffindex, VSRG::ScoreType st)
	{
		ScoreRow ret;
		prepareBest(stBestType, songhash, diffindex);

		sqlite3_bind_int(
			stBestType,
			sqlite3_bind_parameter_index(stBestType, "$type"),
			st
		);

		if (sqlite3_step(stBestType) == SQLITE_ROW) {
			ret = toScoreRow(stBestType);
		}

		sqlite3_reset(stBestType);
		return ret;
	}

	ScoreRow ScoreDatabase::GetBestScoreForSongEX(std::string songhash, int diffindex)
	{
		ScoreRow ret;
		prepareBest(stBestEx, songhash, diffindex);
		if (sqlite3_step(stBestEx) == SQLITE_ROW) {
			ret = toScoreRow(stBestEx);
		}

		sqlite3_reset(stBestEx);
		return ret;
	}

	ScoreRow ScoreDatabase::GetBestScoreForSongRank(std::string songhash, int diffindex)
	{
		ScoreRow ret;
		prepareBest(stBestRank, songhash, diffindex);
		if (sqlite3_step(stBestRank) == SQLITE_ROW) {
			ret = toScoreRow(stBestRank);
		}

		sqlite3_reset(stBestRank);
		return ret;
	}

	std::vector<ScoreRow> ScoreDatabase::GetAllScoresForSong(std::string songhash, int diffindex)
	{
		auto ret = std::vector<ScoreRow>();

		prepareBest(stAllScores, songhash, diffindex);
		while (sqlite3_step(stAllScores) == SQLITE_ROW) {
			ret.push_back(toScoreRow(stAllScores));
		}

		sqlite3_reset(stAllScores);
		return ret;
	}


}