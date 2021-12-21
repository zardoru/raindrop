#pragma once

#include <rmath.h>
#include <game/Gauge.h>

namespace rd {
    template<class Parameter>
    class GaugeLR2 : public Gauge {
        double rate = 1;
        bool failed = false;
    public:
        void DefaultSetup() override { Setup(100, 100, 0); failed = false; }

        void Setup(double total, long long max_notes, double strictness) override {
            rate = Parameter::setup(total, max_notes);
        }

        void Reset() override {
            lifebar_amount = Parameter::init_value;
            failed = false;
        }

        double GetGaugeValue() override {
            return lifebar_amount / 100.0;
        }

        bool HasDelayedFailure() override {
            return Parameter::fail_immediate;
        }

        bool HasFailed(bool song_ended) override {
            if (HasDelayedFailure())
                return song_ended && lifebar_amount < Parameter::pass_threshold;
            else
                return lifebar_amount <= 0;
        }

        void Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) override {
            if (skj == SKJ_NONE || skj == SKJ_TICK) return;
            if (failed) return; /* stop updating */

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

            /* block any further updates */
            if (!HasDelayedFailure() && lifebar_amount < Parameter::pass_threshold)
                failed = true;
        }
    };

    namespace lr2 {
        struct NoIncrementChange {
            static void modify_inc(double lifebar_amount, double &inc) {}
        };

        struct SlowDecrementChange {
            static void modify_inc(double lifebar_amount, double &inc) {
                /* lr2oraja gaugeproperty.java guts */
                // decrease damage below 32%
                if (lifebar_amount < 32 && inc < 0)
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
            static constexpr struct {
                float threshold;
                float value;
            } fix1[] = {
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

            static constexpr struct {
                long long threshold;
                float base;
                float mult;
            } fix2[] = {
                    /* threshold, base, mult */
                    {1000, 1, 0.002},
                    {500, 2, 0.004},
                    {250, 3, 0.008},
                    {125, 4, 1./65.},
                    {60, 5, 0.2/3.0},
                    {30, 8, 0.2},
                    {21, 10, 0}
            };

            static double setup(double total, long long max_notes) {
                /* lr2oraja groovegauge.java */
                double f1 = 10;
                int i = 0;
                while (i < 10) {
                    if (total >= fix1[i].threshold) {
                        f1 = fix1[i].value;
                        break;
                    }
                    i++;
                }

                double f2 = 1.0;
                int j = 0;
                while (j < 7) {
                    auto f2v = fix2[j];

                    if (max_notes < f2v.threshold) {
                        f2 = f2v.base + f2v.mult * (f2v.threshold - max_notes);
                    }
                    j++;
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
        static constexpr bool fail_immediate = false;
    };

    typedef GaugeLR2<AssistLr2GaugeParameters> GaugeLR2Assist;

    struct EasyLr2GaugeParameters : public lr2::NoIncrementChange, public lr2::ModifyTotal {
        static constexpr double pass_threshold = 80;
        static constexpr double min_value = 2;
        static constexpr double init_value = 20;
        static constexpr double early_miss_decrement = -1.6;
        static constexpr double late_miss_decrement = -4.8;
        static constexpr double per_judgment_increments[] = {1.2, 1.2, 1.2, 0.6, -3.2, -3.2};
        static constexpr bool fail_immediate = false;
    };

    typedef GaugeLR2<EasyLr2GaugeParameters>  GaugeLR2Easy;

    struct NormalLr2GaugeParameters : public lr2::NoIncrementChange, public lr2::ModifyTotal {
        static constexpr double pass_threshold = 80;
        static constexpr double min_value = 2;
        static constexpr double init_value = 20;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -6.0;
        static constexpr double per_judgment_increments[] = {1.0, 1.0, 1.0, 0.5, -4.0, -4.0};
        static constexpr bool fail_immediate = false;
    };

    typedef GaugeLR2<NormalLr2GaugeParameters>  GaugeLR2Normal;

    struct HardLr2GaugeParameters : public lr2::SlowDecrementChange, public lr2::ModifyDamage {
        static constexpr double pass_threshold = 2;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -10.0;
        static constexpr double per_judgment_increments[] = {0.1, 0.1, 0.1, 0.05, -6.0, -6.0};
        static constexpr bool fail_immediate = true;
    };

    typedef GaugeLR2<HardLr2GaugeParameters> GaugeLR2Hard;

    struct ExHardLr2GaugeParameters : public lr2::SlowDecrementChange, public lr2::ModifyDamage {
        static constexpr double pass_threshold = 2;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -20.0;
        static constexpr double per_judgment_increments[] = {0.1, 0.1, 0.1, 0.05, -12.0, -12.0};
        static constexpr bool fail_immediate = true;
    };

    typedef GaugeLR2<ExHardLr2GaugeParameters> GaugeLR2ExHard;

    struct HazardLr2GaugeParameters : public lr2::NoIncrementChange, public lr2::ModifyNone {
        static constexpr double pass_threshold = 2;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -10.0;
        static constexpr double late_miss_decrement = -100.0;
        static constexpr double per_judgment_increments[] = {0.15, 0.15, 0.06, 0, -100.0, -100.0};
        static constexpr bool fail_immediate = true;
    };

    typedef GaugeLR2<HazardLr2GaugeParameters> GaugeLr2Hazard;

    struct ClassLr2GaugeParameters : public lr2::SlowDecrementChange, public lr2::ModifyNone {
        static constexpr double pass_threshold = 2;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -3.0;
        static constexpr double per_judgment_increments[] = {0.10, 0.10, 0.10, 0.05, -2.0, -2.0};
        static constexpr bool fail_immediate = true;
    };

    typedef GaugeLR2<ClassLr2GaugeParameters> GaugeLr2Class;

    struct ExClassLr2GaugeParameters : public lr2::SlowDecrementChange, public lr2::ModifyNone {
        static constexpr double pass_threshold = 2;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -10.0;
        static constexpr double per_judgment_increments[] = {0.10, 0.10, 0.10, 0.05, -6.0, -6.0};
        static constexpr bool fail_immediate = true;
    };

    typedef GaugeLR2<ExClassLr2GaugeParameters> GaugeLr2ExClass;

    struct ExHardClassLr2GaugeParameters : public lr2::NoIncrementChange, public lr2::ModifyNone {
        static constexpr double pass_threshold = 2;
        static constexpr double min_value = 0;
        static constexpr double init_value = 100;
        static constexpr double early_miss_decrement = -2.0;
        static constexpr double late_miss_decrement = -20.0;
        static constexpr double per_judgment_increments[] = {0.10, 0.10, 0.10, 0.05, -12.0, -12.0};
        static constexpr bool fail_immediate = true;
    };

    typedef GaugeLR2<ExHardClassLr2GaugeParameters> GaugeLr2ExHardClass;
}