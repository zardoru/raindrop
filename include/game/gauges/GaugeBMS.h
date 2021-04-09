
#pragma once

#include <rmath.h>
#include "../Gauge.h"

namespace rd {
    template<typename GaugeParameters>
    class GaugeBMS : public Gauge {
    protected:
        double lifebar_increment{}, lifebar_decrement{};
    public:
        void DefaultSetup() override {
            Setup(100, 100, 0);
        }

        /* relative_total: #TOTAL / max_notes */
        void Setup(double total, double max_notes, double strictness) override {
            lifebar_increment = Clamp(
                    total / max_notes / GaugeParameters::increase_total_divider,
                    GaugeParameters::min_increment,
                    GaugeParameters::max_increment
            );
            lifebar_decrement = Clamp(
                    total / max_notes / GaugeParameters::decrease_total_divider,
                    GaugeParameters::min_decrement,
                    GaugeParameters::max_decrement
            );
        };

        void Reset() override {
            lifebar_amount = GaugeParameters::reset_value;
        }

        void Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) override {
            if (skj == SKJ_NONE) return;
            if (skj == SKJ_MINE) {
                lifebar_amount -= mine_value;
            } else if (skj > SKJ_W3) { /* bad, miss */
                if (is_early) // miss tier 1 (early miss)
                    lifebar_amount -= lifebar_decrement * GaugeParameters::early_miss_mult;
                else // miss tier 2 (late miss)
                    lifebar_amount -= lifebar_decrement * GaugeParameters::late_miss_mult;
            } else if (skj <= SKJ_W3)  { /* xgreat, pgreat, great*/
                lifebar_amount += lifebar_increment;
            }

            lifebar_amount = Clamp(lifebar_amount, 0.0, 1.0);
        }

        bool HasFailed(bool song_ended) override {
            if (GaugeParameters::pass_threshold != 0)
                return song_ended && lifebar_amount < GaugeParameters::pass_threshold;
            else
                return lifebar_amount <= 0;
        }

        bool HasDelayedFailure() override {
            return GaugeParameters::pass_threshold != 0;
        }

    };

    struct GrooveGaugeParameters {
        static constexpr double increase_total_divider = 100.0; /* higher this is, lower health gain */
        static constexpr double decrease_total_divider = 10.0; /* higher this is, lower health loss */
        static constexpr double min_increment = 0.002;
        static constexpr double max_increment = 0.8;
        static constexpr double min_decrement = 0.01;
        static constexpr double max_decrement = 0.02;
        static constexpr double reset_value = 0.2; /* value to reset lifebar to */
        /* if != 0, you need this much health to pass the chart at the end */
        static constexpr double pass_threshold = 0.8;
        static constexpr double early_miss_mult = 1;
        static constexpr double late_miss_mult = 3;
    };

    typedef GaugeBMS<GrooveGaugeParameters> GaugeGroove;

    struct EasyGaugeParameters {
        static constexpr double increase_total_divider = 50.0; /* higher this is, lower health gain */
        static constexpr double decrease_total_divider = 12.0; /* higher this is, lower health loss */
        static constexpr double min_increment = 0.004;
        static constexpr double max_increment = 0.8;
        static constexpr double min_decrement = 0.00;
        static constexpr double max_decrement = 0.02;
        static constexpr double reset_value = 0.2; /* value to reset lifebar to */
        /* if != 0, you need this much health to pass the chart at the end */
        static constexpr double pass_threshold = 0.8;
        static constexpr double early_miss_mult = 1;
        static constexpr double late_miss_mult = 3;
    };

    typedef GaugeBMS<EasyGaugeParameters> GaugeEasy;

    struct SurvivalGaugeParameters {
        static constexpr double increase_total_divider = 200.0; /* higher this is, lower health gain */
        static constexpr double decrease_total_divider = 7.0; /* higher this is, lower health loss */
        static constexpr double min_increment = -std::numeric_limits<double>::infinity();
        static constexpr double max_increment = std::numeric_limits<double>::infinity();
        static constexpr double min_decrement = 0.02;
        static constexpr double max_decrement = 0.15;
        static constexpr double reset_value = 1; /* value to reset lifebar to */
        /* if != 0, you need this much health to pass the chart at the end */
        static constexpr double pass_threshold = 0;
        static constexpr double early_miss_mult = 1;
        static constexpr double late_miss_mult = 3;
    };

    typedef GaugeBMS<SurvivalGaugeParameters> GaugeSurvival;

    struct ExHardGaugeParameters {
        static constexpr double increase_total_divider = 200.0; /* higher this is, lower health gain */
        static constexpr double decrease_total_divider = 3.0; /* higher this is, lower health loss */
        static constexpr double min_increment = -std::numeric_limits<double>::infinity();
        static constexpr double max_increment = std::numeric_limits<double>::infinity();
        static constexpr double min_decrement = 0.03;
        static constexpr double max_decrement = 0.3;
        static constexpr double reset_value = 1; /* value to reset lifebar to */
        /* if != 0, you need this much health to pass the chart at the end */
        static constexpr double pass_threshold = 0;
        static constexpr double early_miss_mult = 1;
        static constexpr double late_miss_mult = 3;
    };

    typedef GaugeBMS<ExHardGaugeParameters> GaugeExHard;

    struct DeathGaugeParameters {
        static constexpr double increase_total_divider = 1.0; /* higher this is, lower health gain */
        static constexpr double decrease_total_divider = 1.0; /* higher this is, lower health loss */
        static constexpr double min_increment = 0;
        static constexpr double max_increment = 0;
        static constexpr double min_decrement = 1;
        static constexpr double max_decrement = 1;
        static constexpr double reset_value = 1; /* value to reset lifebar to */
        static constexpr double pass_threshold = 0;
        static constexpr double early_miss_mult = 0;
        static constexpr double late_miss_mult = 1;
    };

    typedef GaugeBMS<DeathGaugeParameters> GaugeDeath;
}