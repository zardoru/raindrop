#pragma once

#include "../GameConstants.h"
#include "../ScoreSystem.h"

namespace rd {
    class ScoreSystemOsuMania : public ScoringSystem {
        long long osu_points;
        double osu_bonus_points;
        int bonus_counter;
    public:
        void reset() override;

        void update(ScoreKeeperJudgment skj, bool use_w0) override;

        long long get_current_score(long long max_notes, bool use_w0) const override;

        long long get_max_score(long long max_notes, bool use_w0) override;
    };

    // 0-100%
    class ScoreSystemOsuManiaAccuracy : public ScoringSystem {
        long long osu_accuracy;
    public:
        void reset() override;

        void update(ScoreKeeperJudgment skj, bool use_w0) override;

        long long get_current_score(long long max_notes, bool use_w0) const override;

        long long get_max_score(long long max_notes, bool use_w0) override;
    };
}
