
#pragma once


struct ScoreRow {
    long long rank_score;
    int exscore;
    int score;
    float gauge;
    int hits;
    int max_combo;
    int judgments[6];
    int misses;
    float avghit;
    float stdev;
    double offset;
    double judgeoffset;
    PlayscreenParameters play_opts;

    ScoreRow() {
        exscore = score = 0;
        gauge = 0;
        avghit = stdev = 0;
        rank_score = 0;
        memset(judgments, 0, sizeof(judgments));
        hits = misses = 0;
        offset = judgeoffset = 0;
        max_combo = 0;
    }

    // returns true if the score is to be considered
    // anything that was actually played.
    bool IsPlayedScore() {
        // enough things are zero to say we didn't play this thing
        if (hits == misses && hits == 0) {
            if (score == 0 || play_opts.GaugeType == rd::LT_AUTO)
                return false;
        }

        return true;
    }
};

struct sqlite3;
struct sqlite3_stmt;

class ScoreDatabase {
    sqlite3* db;
    sqlite3_stmt *stAddScore,
        *stBestEx,
        *stBestRank,
        *stAllScores,
        *stAddHash;

    void AddHash(std::string hash);

    void CleanupStatements();
public:
    ScoreDatabase();
    ~ScoreDatabase();
    void Open(std::filesystem::path path);
    void AddScore(
        std::string charthash,
        int diffindex,
        PlayscreenParameters params,
        rd::ScoreKeeper keeper,
        double offset, // note time displacement
        double judgeoffset); // judgment time displacement

    ScoreRow GetBestScoreForSong(std::string songhash, int diffindex, rd::ScoreType st);
    ScoreRow GetBestScoreForSongEX(std::string songhash, int diffindex);
    ScoreRow GetBestScoreForSongRank(std::string songhash, int diffindex);

    std::vector<ScoreRow> GetAllScoresForSong(std::string songhash, int diffindex);
};
