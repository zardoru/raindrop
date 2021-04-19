#pragma once

#include "../GameConstants.h"
#include "../ScoreSystem.h"


namespace rd {
    class ScoreSystemO2Jam : public ScoringSystem {
        char pills;

        long long coolcombo;
        long long o2_score;
        long long jams;
        long long jam_jchain;
    public:
        void Reset() override;

        void Update(ScoreKeeperJudgment skj, bool use_w0) override;

        long long GetCurrentScore(long long max_notes, bool use_w0) const override;

        long long GetMaxScore(long long max_notes, bool use_w0) override;

        ScoreKeeperJudgment MutateJudgment(ScoreKeeperJudgment skj);

        long long GetCoolCombo() const;
        long long GetJams() const;
        char GetPills() const;
        long long GetJamChain() const;
    };
}