#pragma once

#include "../GameConstants.h"
#include "../ScoreSystem.h"

namespace rd {
    /*

    EXP^2 system

    The way this system works is similar to how combo scoring works in Beatmania.

    The max score for this system is 1,200,000 points, which is a normalized version of another score.

    The score that a note receives is based on your current "point combo".

    - W3 increases the point combo by  2.
    - W2 increases the point combo by  3.
    - W1 increases the point combo by  4.

    This point combo starts at the beginning of a song and has a maximum bound of 100.
    However, the point combo resets to a maximum of 96 after a note is judged, so a W3 cannot get more than 98 points,
     and a W2 cannot get more than 99.

    Also, the point combo resets to 0 if a note is missed.

    */

    class ScoreSystemExp : public ScoringSystem {
    protected:
        long long exp_combo;
        long long exp_combo_pts;

        long long exp_hit_score;

        long long GetMaxComboPts(long long max_notes) const;
    public:
        void Reset() override;

        void Update(ScoreKeeperJudgment skj, bool use_w0) override;

        long long GetCurrentScore(long long max_notes, bool use_w0) const override;

        long long GetMaxScore(long long max_notes, bool use_w0) override;
    };

    class ScoreSystemExp3 : public ScoreSystemExp {
    public:
        long long GetCurrentScore(long long max_notes, bool use_w0) const override;

        long long GetMaxScore(long long max_notes, bool use_w0) override;
    };

    class ScoreSystemRank : public ScoringSystem {
        long long rank_w0_count;
        long long rank_w1_count;
        long long rank_w2_count;
        long long rank_w3_count;

        long long judged_notes;
    public:
        void Reset() override;

        void Update(ScoreKeeperJudgment skj, bool use_w0) override;

        long long GetCurrentScore(long long max_notes, bool use_w0) const override;

        long long GetMaxScore(long long max_notes, bool use_w0) override;

        int GetRank() const;
    };
}