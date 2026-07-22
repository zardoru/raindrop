#pragma once

namespace rd {
    class GaugeOsuMania : public Gauge {
        float hp_;
        std::array<double, 7> hp_change_;
        double ln_tick_fill_;
    public:
        void reset() override;
        void default_setup() override;
        void setup(double total, long long max, double strictness) override;
        void update(ScoreKeeperJudgment skj, bool is_early, float mine_value) override;
    };
}