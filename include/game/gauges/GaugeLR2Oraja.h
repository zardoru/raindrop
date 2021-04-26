#pragma once

#include <rmath.h>
#include <game/Gauge.h>

namespace rd {
    template<class Parameter>
    class GaugeLR2 : public Gauge {
        double rate = 1;
    public:
        void DefaultSetup() override { Setup(100, 100, 0); }

        void Setup(double total, long long max_notes, double strictness) override {
            rate = Parameter::setup(total, max_notes);
        }

        void Reset() override {
            lifebar_amount = Parameter::init_value;
        }

        double GetGaugeValue() override {
            return lifebar_amount / 100.0;
        }

        bool HasDelayedFailure() override {
            return Parameter::pass_threshold != 0;
        }

        bool HasFailed(bool song_ended) override {
            if (HasDelayedFailure())
                return song_ended && lifebar_amount < Parameter::pass_threshold;
            else
                return lifebar_amount <= 0;
        }

        void Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) override {
            if (skj == SKJ_NONE || skj == SKJ_TICK) return;

            double gauge_inc = 0;
            if (skj > SKJ_MINE || skj < SKJ_W0) {
                if (skj == SKJ_MINE) {
                    gauge_inc = mine_value * 100;
                } else
                    return;
            }

            if (skj == SKJ_MISS) {
                if (is_early)
                    gauge_inc += Parameter::early_miss_decrement;
                else
                    gauge_inc += Parameter::late_miss_decrement;
            } else {
                gauge_inc += Parameter::per_judgment_increments[skj];
            }

            gauge_inc *= rate;
            Parameter::modify_inc(lifebar_amount, gauge_inc);

            lifebar_amount += gauge_inc;
            lifebar_amount = Clamp(lifebar_amount, Parameter::min_value, 100.0);
        }
    };

    namespace lr2 {
        struct NoIncrementChange {
            static void modify_inc(double lifebar_amount, double &inc) {}
        };

        struct SlowDecrementChange {
            static void modify_inc(double lifebar_amount, double &inc) {
                /* lr2oraja gaugeproperty.java guts */
                if (lifebar_amount < 30 && inc < 0)
                    inc *= 0.6;
            }
        };

        struct ModifyTotal {
            static double setup(double total, long long max_notes) {
                return total / double(max_notes);
            }
        };

        struct ModifyNone {
            static double setup(double total, long long max_notes) { return 1; }
        };

        struct ModifyDamage {
            static constexpr float fix1[10][2] = {
                    /* threshold, value */
                    {240, 1},
                    {230, 1.11},
                    {210, 1.25},
                    {200, 1.5},
                    {180, 1.666},
                    {160, 2},
                    {150, 2.5},
                    {130, 3.333},
                    {120, 5}
            };

            static double setup(double total, long long max_notes) {
                /* lr2oraja groovegauge.java */
                double f1 = 10;
                int i = 0;
                while (i < 10) {
                    if (total >= fix1[i][0]) { /* total greater than threshold, set f1 to value */
                        f1 = fix1[i][1];
                        break;
                    }
                    i++;
                }

                long long note = 1000;
                double mod = 0.002;
                double f2 = 1.0;
                while (note > max_notes || note > 1) {
                    f2 += mod * (note - std::max(max_notes, note / 2));
                    note /= 2;
                    mod *= 2.0;
                }

                return std::max(f1, f2);
            }
        };

    }

    struct AssistLr2GaugeParameters : public lr2::NoIncrementChange, public lr2::ModifyTotal {
        static constexpr double pass_threshold = 60;
        static constexpr double min_value = 2;
        static constexpr double init_value = 20;
        static constexpr double early_miss_decrement = -1.6;
        static constexpr double late_miss_decrement = -4.8;
        static constexpr double per_judgment_increments[] = {1.2, 1.2, 1.2, 0.6, -3.2, -3.2};
    };

    typedef GaugeLR2<AssistLr2GaugeParameters> GaugeLR2Assist;

    struct EasyLr2GaugeParameters : public lr2::NoIncrementChange, public lr2::ModifyTotal {
        static constexpr double pass_threshold = 80;
        static constexpr double min_value = 2;
        static constexpr double init_value = 20;
        static constexpr double early_miss_decrement = -1.6;
        static constexpr double late_miss_decrement = -4.8;
        static constexpr double per_judgment_increments[] = {1.2, 1.2, 1.2, 0.6, -3.2, -3.2};
    };

    typedef GaugeLR2<EasyLr2GaugeParameters>  GaugeLR2Easy;

    struct NormalLr2GaugeParameters : public lr2::NoIncrementChange, public lr2::ModifyTotal {
        static constexpr double pass_threshold = 80;
        static constexpr double min_value = 2;
        static constexpr double init_value = 20;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -6.0;
        static constexpr double per_judgment_increments[] = {1.0, 1.0, 1.0, 0.5, -4.0, -4.0};
    };

    typedef GaugeLR2<NormalLr2GaugeParameters>  GaugeLR2Normal;

    struct HardLr2GaugeParameters : public lr2::SlowDecrementChange, public lr2::ModifyDamage {
        static constexpr double pass_threshold = 0;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -10.0;
        static constexpr double per_judgment_increments[] = {0.1, 0.1, 0.1, 0.05, -6.0, -6.0};
    };

    typedef GaugeLR2<HardLr2GaugeParameters> GaugeLR2Hard;

    struct ExHardLr2GaugeParameters : public lr2::SlowDecrementChange, public lr2::ModifyDamage {
        static constexpr double pass_threshold = 0;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -20.0;
        static constexpr double per_judgment_increments[] = {0.1, 0.1, 0.1, 0.05, -12.0, -12.0};
    };

    typedef GaugeLR2<ExHardLr2GaugeParameters> GaugeLR2ExHard;

    struct HazardLr2GaugeParameters : public lr2::NoIncrementChange, public lr2::ModifyNone {
        static constexpr double pass_threshold = 0;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -10.0;
        static constexpr double late_miss_decrement = -100.0;
        static constexpr double per_judgment_increments[] = {0.15, 0.15, 0.06, 0, -100.0, -100.0};
    };

    typedef GaugeLR2<HazardLr2GaugeParameters> GaugeLr2Hazard;

    struct ClassLr2GaugeParameters : public lr2::SlowDecrementChange, public lr2::ModifyNone {
        static constexpr double pass_threshold = 0;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -3.0;
        static constexpr double per_judgment_increments[] = {0.10, 0.10, 0.10, 0.05, -2.0, -2.0};
    };

    typedef GaugeLR2<ClassLr2GaugeParameters> GaugeLr2Class;

    struct ExClassLr2GaugeParameters : public lr2::SlowDecrementChange, public lr2::ModifyNone {
        static constexpr double pass_threshold = 0;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -10.0;
        static constexpr double per_judgment_increments[] = {0.10, 0.10, 0.10, 0.05, -6.0, -6.0};
    };

    typedef GaugeLR2<ExClassLr2GaugeParameters> GaugeLr2ExClass;

    struct ExHardClassLr2GaugeParameters : public lr2::NoIncrementChange, public lr2::ModifyNone {
        static constexpr double pass_threshold = 0;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -20.0;
        static constexpr double per_judgment_increments[] = {0.10, 0.10, 0.10, 0.05, -12.0, -12.0};
    };

    typedef GaugeLR2<ExHardClassLr2GaugeParameters> GaugeLr2ExHardClass;
}