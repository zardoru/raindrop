#pragma once

#include <game/Gauge.h>

namespace rd {
    class GaugeO2Jam : public Gauge {
    protected:
        double increment{}, decrement{}, decrement_bad{};
    public:
        void DefaultSetup() override;

        void Reset() override;

        void Setup (double total, long long max_notes, double strictness) override;

        void Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) override;
    };


}