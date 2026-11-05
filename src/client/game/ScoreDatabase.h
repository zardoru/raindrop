
#pragma once


struct ScoreRow {
    long long rank_score;
    int exscore;
    int score;
    float gauge;
    int hits;
    int max_combo;
    int judgments[6]{};
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
        hits = misses = 0;
        offset = judgeoffset = 0;
        max_combo = 0;
    }

    // returns true if the score is to be considered
    // anything that was actually played.
    bool is_played_score() const {
        // enough things are zero to say we didn't play this thing
        if (hits == misses && hits == 0) {
            if (score == 0 || play_opts.gauge_type == rd::LT_AUTO)
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

    void add_hash(std::string hash);

    void cleanup_statements();
public:
    ScoreDatabase();
    ~ScoreDatabase();
    void open(std::filesystem::path path);
    void add_score(
        std::string charthash,
        int diffindex,
        PlayscreenParameters params,
        rd::ScoreKeeper keeper,
        double offset, // note time displacement
        double judgeoffset); // judgment time displacement

    ScoreRow get_best_score_for_song(std::string songhash, int diffindex, rd::ScoreType st);
    ScoreRow get_best_score_for_song_ex(std::string songhash, int diffindex);
    ScoreRow get_best_score_for_song_rank(std::string songhash, int diffindex);

    std::vector<ScoreRow> get_all_scores_for_song(std::string songhash, int diffindex);
};
