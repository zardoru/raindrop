#include <rmath.h>
#include <game/Gauge.h>
#include <game/gauges/GaugeO2Jam.h>
#include <game/gauges/GaugeStepmania.h>
#include <game/gauges/GaugeOsuMania.h>
#include <game/gauges/GaugeLR2Oraja.h>

using namespace rd;

bool Gauge::HasFailed(bool song_ended) {
    return lifebar_amount <= 0;
}

double Gauge::GetGaugeValue() {
    return lifebar_amount;
}

double Gauge::GetGaugeDisplayValue() {
    return lifebar_amount;
}

bool Gauge::HasDelayedFailure() {
    return false;
}

void Gauge::Setup(double total, long long max_notes, double strictness) {
    // stub
}

void Gauge::DefaultSetup() {
   // stub
}

void GaugeO2Jam::Setup(double total, long long max_notes, double strictness) {
    // Thanks to Entozer for giving this information.
    if (strictness == 0) { // EX
        increment = 0.3;
        increment_good = 0.2;
        decrement_bad = -1;
        decrement_miss = -5;
    } else if (strictness == 1) { // NX
        increment = 0.2;
        increment_good = 0.1;
        decrement_bad = -0.7;
        decrement_miss = -4;
    } else { // HX
        increment = 0.1;
        increment_good = 0.0;
        decrement_bad = -0.5;
        decrement_miss = -3;
    }
}

void GaugeO2Jam::Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) {
    if (skj == SKJ_NONE) return;
    if (skj == SKJ_TICK) return;
    if (skj <= SKJ_W1) // only COOLs restore o2jam lifebar
        lifebar_amount += increment;
    else if (skj == SKJ_W2)
        lifebar_amount += increment_good;
    else if (skj == SKJ_MISS)
        lifebar_amount -= decrement_miss;
    else if (skj >= SKJ_W3) // BADs get some HP from you,
        lifebar_amount -= decrement_bad;
}

void GaugeO2Jam::Reset() {
    lifebar_amount = 100;
}

void GaugeO2Jam::DefaultSetup() {
    Setup(0, 0, 2); // HX
}

double GaugeO2Jam::GetGaugeValue() {
    return lifebar_amount / 100.0;
}

void GaugeStepmania::Reset() { lifebar_amount = 0.5; }

void GaugeStepmania::Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) {
    if (skj == SKJ_TICK) return;
    if (skj == SKJ_MINE) {
        lifebar_amount -= mine_value;
    } else {
        // in range - we rely on the enum values lining up with the array.
        if (skj >= SKJ_W0 && skj <= SKJ_MISS)
            lifebar_amount += increments[skj];

        if (skj == SKJ_MISS && is_early)
            lifebar_amount -= increments[SKJ_MISS] / 2.0; // early miss compensation, half the WMiss value
    }

    lifebar_amount = Clamp(lifebar_amount, 0.0, 1.0);
}

void GaugeOsuMania::DefaultSetup() {
    Setup(0, 0, 8);
}

void GaugeOsuMania::Setup(double total, long long int max, double strictness) {
    HP = Clamp(strictness, 0.0, 10.0);

    double _whole;
    const auto fraction = modf(HP, &_whole);
    const auto whole = (long long)_whole;
    static constexpr std::array<double, 11> HPM = { /* HP multipliers */
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



    // interpolate the HPM
    auto hpm_value = HPM[whole];
    if (fraction > 0 && whole + 1 <= 10)
        hpm_value += (HPM[whole + 1] - HPM[whole]) * fraction;

    // tick fill
    ln_tick_fill = (0.5 - HP*0.05) * HP / 200.0;

    // judgment fill
    hp_change[SKJ_W0] = hp_change[SKJ_W1] = (1.1 - HP * 0.1);
    hp_change[SKJ_W2] = 0.5 - HP * 0.05;
    hp_change[SKJ_W3] = 0.8 - HP * 0.08;
    hp_change[SKJ_W4] = 0;
    hp_change[SKJ_W5] = -(HP+1)*0.32;
    hp_change[SKJ_MISS] = -(HP+1)*1.5;

    /* normalize to 0-1 scale, apply HPM */
    for (int i = SKJ_W0; i <= SKJ_MISS; i++) {

        // do not apply HPM to 50s nor misses
        // because it doesn't make sense to "increase" them as HP goes from 10 to 0.
        if (i < SKJ_W5)
            hp_change[i] *= hpm_value;

        hp_change[i] /= 200.0;
    }
}

void GaugeOsuMania::Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) {
    if (skj == SKJ_NONE) return;
    if (skj == SKJ_TICK) lifebar_amount += ln_tick_fill;
    if (skj >= SKJ_W0 && skj <= SKJ_MISS)
        lifebar_amount += hp_change[skj];
    lifebar_amount = Clamp(lifebar_amount, 0.0, 1.0);
}

void GaugeOsuMania::Reset() {
    lifebar_amount = 1;
}
