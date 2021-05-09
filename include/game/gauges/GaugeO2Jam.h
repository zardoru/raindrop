#pragma once

#include <game/Gauge.h>

namespace rd {
    class GaugeO2Jam : public Gauge {
    protected:
        double increment{}, increment_good{}, decrement_bad{}, decrement_miss{};
    public:
        void DefaultSetup() override;

        void Reset() override;

        void Setup (double total, long long max_notes, double strictness) override;

        void Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) override;

        virtual double GetGaugeValue() override;
    };


}