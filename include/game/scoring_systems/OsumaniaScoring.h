#pragma once

#include "../GameConstants.h"
#include "../ScoreSystem.h"

namespace rd {
    class ScoreSystemOsuMania : public ScoringSystem {
        long long osu_points;
        double osu_bonus_points;
        int bonus_counter;
    public:
        void Reset() override;

        void Update(ScoreKeeperJudgment skj, bool use_w0) override;

        long long GetCurrentScore(long long max_notes, bool use_w0) const override;

        long long GetMaxScore(long long max_notes, bool use_w0) override;
    };

    // 0-100%
    class ScoreSystemOsuManiaAccuracy : public ScoringSystem {
        long long osu_accuracy;
    public:
        void Reset() override;

        void Update(ScoreKeeperJudgment skj, bool use_w0) override;

        long long GetCurrentScore(long long max_notes, bool use_w0) const override;

        long long GetMaxScore(long long max_notes, bool use_w0) override;
    };
}
