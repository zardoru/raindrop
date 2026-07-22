#pragma once

#include "../GameConstants.h"
#include "../ScoreSystem.h"

namespace rd {
    class ScoreSystemBMS : public ScoringSystem {
        long long bms_combo;
        long long bms_combo_pts;
        long long bms_dance_pts;

        static long long GetMaxComboPts(long long notes);
    public:
        void reset() override;
        void update(ScoreKeeperJudgment skj, bool use_w0) override;
        long long get_current_score(long long max_notes, bool use_w0) const override;
        long long get_max_score(long long max_notes, bool use_w0) override;
    };

    class ScoreSystemEX : public ScoringSystem {
        long long ex_score;
    public:
        void reset() override;
        void update(ScoreKeeperJudgment skj, bool use_w0) override;
        long long get_current_score(long long max_notes, bool use_w0) const override;
        long long get_max_score(long long max_notes, bool use_w0) override;
        rd::PacemakerType GetRank(long long max_notes) const;
    };

    class ScoreSystemLR2 : public ScoringSystem {
        long long lr2_dance_pts;
    public:
        void reset() override;
        void update(ScoreKeeperJudgment skj, bool use_w0) override;
        long long get_current_score(long long max_notes, bool use_w0) const override;
        long long get_max_score(long long max_notes, bool use_w0) override;
    };
}