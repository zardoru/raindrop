#include <rmath.h>
#include <game/Gauge.h>
#include <game/gauges/GaugeBMS.h>
#include <game/GameConstants.h>
#include <game/gauges/GaugeO2Jam.h>
#include <game/gauges/GaugeStepmania.h>

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

void Gauge::Setup(double total, double max_notes, double strictness) {
    // stub
}

void Gauge::DefaultSetup() {
   // stub
}

void GaugeO2Jam::Setup(double total, double max_notes, double strictness) {
    // Thanks to Entozer for giving this information.
    if (strictness == 0) { // EX
        increment = (19.0 / 20.0) / 290.0;
        decrement = 1.0 / 20.0;
        decrement_bad = 1.0 / 100.0;
    } else if (strictness == 1) { // NX
        increment = (24.0 / 25.0) / 470.0;
        decrement = 1.0 / 25.0;
        decrement_bad = 1.0 / 144.0;
    } else { // HX
        increment = (33.0 / 34.0) / 950.0;
        decrement = 1.0 / 34.0;
        decrement_bad = 1.0 / 200.0;
    }
}

void GaugeO2Jam::Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) {
    if (skj == SKJ_NONE) return;
    if (skj <= SKJ_W1) // only COOLs restore o2jam lifebar
        lifebar_amount += increment;
    else if (skj == SKJ_MISS)
        lifebar_amount -= decrement;
    else if (skj >= SKJ_W3) // BADs get some HP from you,
        lifebar_amount -= decrement_bad;
}

void GaugeO2Jam::Reset() {
    lifebar_amount = 1;
}

void GaugeO2Jam::DefaultSetup() {
    Setup(0, 0, 2); // HX
}

void GaugeStepmania::Reset() { lifebar_amount = 0.5; }

void GaugeStepmania::Update(ScoreKeeperJudgment skj, bool is_early, float mine_value) {
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