#include <cmath>
#include <rmath.h>
#include <game/Gauge.h>
#include <game/gauges/GaugeO2Jam.h>
#include <game/gauges/GaugeStepmania.h>
#include <game/gauges/GaugeOsuMania.h>
#include <game/gauges/GaugeLR2Oraja.h>

using namespace rd;

bool Gauge::has_failed(bool song_ended) {
    return lifebar_amount_ <= 0;
}

double Gauge::get_gauge_value() {
    return lifebar_amount_;
}

double Gauge::get_gauge_display_value() {
    return lifebar_amount_;
}

bool Gauge::has_delayed_failure() {
    return false;
}

void Gauge::setup(double total, long long max_notes, double strictness) {
    // stub
}

void Gauge::default_setup() {
   // stub
}

void GaugeO2Jam::setup(double total, long long max_notes, double strictness) {
    // Thanks to Entozer for giving this information.
    if (strictness == 0) { // EX
        increment_ = 0.3;
        increment_good_ = 0.2;
        decrement_bad_ = -1;
        decrement_miss_ = -5;
    } else if (strictness == 1) { // NX
        increment_ = 0.2;
        increment_good_ = 0.1;
        decrement_bad_ = -0.7;
        decrement_miss_ = -4;
    } else { // HX
        increment_ = 0.1;
        increment_good_ = 0.0;
        decrement_bad_ = -0.5;
        decrement_miss_ = -3;
    }
}

void GaugeO2Jam::update(ScoreKeeperJudgment skj, bool is_early, float mine_value) {
    if (skj == SKJ_NONE) return;
    if (skj == SKJ_TICK) return;
    if (skj <= SKJ_W1) // only COOLs restore o2jam lifebar
        lifebar_amount_ += increment_;
    else if (skj == SKJ_W2)
        lifebar_amount_ += increment_good_;
    else if (skj == SKJ_MISS)
        lifebar_amount_ -= decrement_miss_;
    else if (skj >= SKJ_W3) // BADs get some hp_ from you,
        lifebar_amount_ -= decrement_bad_;
}

void GaugeO2Jam::reset() {
    lifebar_amount_ = 100;
}

void GaugeO2Jam::default_setup() {
    setup(0, 0, 2); // HX
}

double GaugeO2Jam::get_gauge_value() {
    return lifebar_amount_ / 100.0;
}

void GaugeStepmania::reset() { lifebar_amount_ = 0.5; }

void GaugeStepmania::update(ScoreKeeperJudgment skj, bool is_early, float mine_value) {
    if (skj == SKJ_TICK) return;
    if (skj == SKJ_MINE) {
        lifebar_amount_ -= mine_value;
    } else {
        // in range - we rely on the enum values lining up with the array.
        if (skj >= SKJ_W0 && skj <= SKJ_MISS)
            lifebar_amount_ += increments_[skj];

        if (skj == SKJ_MISS && is_early)
            lifebar_amount_ -= increments_[SKJ_MISS] / 2.0; // early miss compensation, half the WMiss value
    }

    lifebar_amount_ = clamp(lifebar_amount_, 0.0, 1.0);
}

void GaugeOsuMania::default_setup() {
    setup(0, 0, 8);
}

void GaugeOsuMania::setup(double total, long long int max, double strictness) {
    hp_ = clamp(strictness, 0.0, 10.0);

    double whole_part;
    const auto fraction = modf(hp_, &whole_part);
    const auto whole = (long long)whole_part;
    static constexpr std::array<double, 11> hpm = { /* hp_ multipliers */
            7.272727273,
            7.142857143,
            7.168458781,
            6.944444444,
            6.802721088,
            6.666666667,
            6.451612903,
            6.024096386,
            5.376344086,
            4.032258065,
            1.111111111
            /*8, // old, wrong values
            7.142857143,
            6.451612903,
            5.555555556,
            4.761904762,
            4,
            3.225806452,
            2.409638554,
            1.612903226,
            0.8064516129,
            0.1111111111,*/
    };



    // interpolate the hpm
    auto hpm_value = hpm[whole];
    if (fraction > 0 && whole + 1 <= 10)
        hpm_value += (hpm[whole + 1] - hpm[whole]) * fraction;

    // tick fill
    ln_tick_fill_ = (0.5 - hp_*0.05) * hp_ / 200.0;

    // judgment fill
    hp_change_[SKJ_W0] = hp_change_[SKJ_W1] = (1.1 - hp_ * 0.1);
    hp_change_[SKJ_W2] = 0.5 - hp_ * 0.05;
    hp_change_[SKJ_W3] = 0.8 - hp_ * 0.08;
    hp_change_[SKJ_W4] = 0;
    hp_change_[SKJ_W5] = -(hp_+1)*0.32;
    hp_change_[SKJ_MISS] = -(hp_+1)*1.5;

    /* normalize to 0-1 scale, apply hpm */
    for (int i = SKJ_W0; i <= SKJ_MISS; i++) {

        // do not apply hpm to 50s nor misses
        // because it doesn't make sense to "increase" them as hp_ goes from 10 to 0.
        if (i < SKJ_W5)
            hp_change_[i] *= hpm_value;

        hp_change_[i] /= 200.0;
    }
}

void GaugeOsuMania::update(ScoreKeeperJudgment skj, bool is_early, float mine_value) {
    if (skj == SKJ_NONE) return;
    if (skj == SKJ_TICK) lifebar_amount_ += ln_tick_fill_;
    if (skj >= SKJ_W0 && skj <= SKJ_MISS)
        lifebar_amount_ += hp_change_[skj];
    lifebar_amount_ = clamp(lifebar_amount_, 0.0, 1.0);
}

void GaugeOsuMania::reset() {
    lifebar_amount_ = 1;
}
