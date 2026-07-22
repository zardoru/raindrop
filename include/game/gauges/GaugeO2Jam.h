#pragma once

#include <game/Gauge.h>

namespace rd {
    class GaugeO2Jam : public Gauge {
    protected:
        double increment_{}, increment_good_{}, decrement_bad_{}, decrement_miss_{};
    public:
        void default_setup() override;

        void reset() override;

        void setup (double total, long long max_notes, double strictness) override;

        void update(ScoreKeeperJudgment skj, bool is_early, float mine_value) override;

        virtual double get_gauge_value() override;
    };


}