
#pragma once

#include <array>
#include <game/Gauge.h>
#include <rmath.h>

namespace rd {
    class GaugeStepmania : public Gauge {
        static constexpr std::array<double, 7> increments = {
            +0.010,  // ridiculous W0
            +0.008,  // marvelous W1
            +0.008,  // perfect W2
            +0.004,  // great W3
            0,       // good W4
            -0.04,   // boo W5
            -0.08    // miss WMiss
        };
    public:
        void Reset() override;
        void Update ( ScoreKeeperJudgment skj, bool is_early, float mine_value) override;
    };


}