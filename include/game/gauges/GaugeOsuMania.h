#pragma once

namespace rd {
    class GaugeOsuMania : public Gauge {
        float HP;
        std::array<double, 7> hp_change;
        double ln_tick_fill;
    public:
        void Reset() override;
        void DefaultSetup() override;
        void Setup(double total, long long max, double strictness) override;
        void Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) override;
    };
}