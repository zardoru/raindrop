#include <rmath.h>
#include <game/TimingWindows.h>
#include <game/timing_windows/TimingWindowsRaindropBMS.h>
#include <game/timing_windows/TimingWindowsStepmania.h>
#include <game/timing_windows/TimingWindowsOsuMania.h>
#include <game/timing_windows/TimingWindowsO2Jam.h>
#include <game/timing_windows/TimingWindowsLR2Oraja.h>

using namespace rd;

/*
 * Timing Windows Base Class
 * */

void TimingWindows::Setup(double strictness, double scale) {
    // stub. might get ignored in favor of a constructor
}

ScoreKeeperJudgment TimingWindows::GetJudgmentForTimeOffset(double time_delta, uint32_t lane, NoteJudgmentPart part) {
    return GetJudgmentFromTimingWindows(judgment_time, time_delta);
}

double TimingWindows::GetEarlyThreshold() const{
    return early_miss_threshold;
}

double TimingWindows::GetLateThreshold() const {
    return late_miss_threshold;
}

void TimingWindows::AddJudgment(ScoreKeeperJudgment skj, bool early_miss) {
    if (skj >= 0 && skj < JUDGMENT_ARRAY_SIZE) {
        judgment_amt[skj] += 1;
        UpdateCombo(skj, !early_miss);
    }
}

uint32_t TimingWindows::GetJudgmentCount(ScoreKeeperJudgment skj) const {
    if (skj >= 0 && skj < JUDGMENT_ARRAY_SIZE)
        return judgment_amt[skj];
    return 0;
}

TimingType TimingWindows::GetTimeUnits() {
    return TT_TIME;
}

TimingWindows::TimingWindows() {
    judgment_amt.fill(0);
    judgment_time.fill(-1); // disable all.
}

void TimingWindows::Reset() {
    judgment_amt.fill(0);
    combo = 0;
    max_combo = 0;
    notes_hit = 0;
}

void TimingWindows::SetWindowSkip(uint32_t window_skip) {
    current_window_skip = Clamp(window_skip, min_window_skip, max_window_skip);
}

void TimingWindows::DefaultSetup() {
    // stub
}

void TimingWindows::UpdateCombo(ScoreKeeperJudgment skj, bool should_break_combo) {
    if (skj <= combo_leniency) {
        notes_hit++;
        combo++;

        max_combo = std::max(combo, max_combo);
    } else {
        if (should_break_combo)
            combo = 0;
    }
}

double TimingWindows::GetEarlyHitCutoff() const {
    return early_hit_threshold;
}

double TimingWindows::GetJudgmentWindow(ScoreKeeperJudgment skj) {
    if (skj >= 0 && skj < judgment_time.size())
        return judgment_time[skj];
    return -1;
}

uint32_t  TimingWindows::GetCombo() const {
    return combo;
}

uint32_t TimingWindows::GetMaxCombo() const {
    return max_combo;
}

uint32_t TimingWindows::GetNotesHit() const {
    return notes_hit;
}

uint32_t TimingWindows::GetWindowSkip() const {
    return current_window_skip;
}

double TimingWindows::GetTickInterval() {
    return 0;
}

ScoreKeeperJudgment
TimingWindows::GetJudgmentFromTimingWindows(const std::array<double, JUDGMENT_ARRAY_SIZE> &windows, double time_delta) const {
    /* assuming time_delta is negative */
    if (time_delta < -early_hit_threshold) {
        if (time_delta >= -early_miss_threshold) {
            return SKJ_MISS;
        }
    } else if (time_delta > late_miss_threshold) {
        return SKJ_MISS;
    } else if (time_delta >= -early_hit_threshold) {
        time_delta = abs(time_delta);

        auto skj = current_window_skip;
        for (auto i = current_window_skip; i < windows.size(); i++) { /* running off the assumption */
            auto wnd = windows[i];
            if (time_delta <= wnd) {
                auto skj_val = (ScoreKeeperJudgment) skj;
                return skj_val;
            }
            skj += 1;
        }
    }

    return SKJ_NONE;
}

bool TimingWindows::UsesTwoJudgesPerHold() const {
    return true;
}

/*
 * Raindrop BMS
 */

void TimingWindowsRaindropBMS::Setup(double strictness, double scale) {
    static constexpr double JudgmentValues[] = {6.4, 16, 40, 100, 250, -1, 625};

    // don't scale any of these
    early_miss_threshold = judgment_time[SKJ_MISS];
    early_hit_threshold = judgment_time[SKJ_W4];
    late_miss_threshold = judgment_time[SKJ_W4];

    for (auto i = 0; i < sizeof(JudgmentValues) / sizeof(double); i++)
        judgment_time[i] = JudgmentValues[i] * scale;

    // account for extremely low judge_window_scale
    judgment_time[SKJ_W0] = std::max(judgment_time[SKJ_W0], 3.2);
    judgment_time[SKJ_W1] = std::max(judgment_time[SKJ_W1], 8.0);

    // cap early hit threshold
    if (judgment_time[SKJ_W4] < 250.0) {
        auto window = std::max(judgment_time[SKJ_W1], judgment_time[SKJ_W4]);
        early_hit_threshold = late_miss_threshold = std::min(window, 250.0);
    }

    current_window_skip = 1;
    max_window_skip = 1;
}

void TimingWindowsRaindropBMS::DefaultSetup() {
    Setup(0, 1.375);
}

void TimingWindowsStepmania::Setup(double strictness, double scale) {
    // Ridiculous is included: J7 Marvelous.
    // No early miss threshold
    // w0, w1, w2, w3, w4, w5
    static constexpr double JudgmentValues[] = {11.25, 22.5, 45, 90, 135, 180};

    late_miss_threshold = 180;
    early_miss_threshold = 180;
    early_hit_threshold = 180;

    for (int i = 0; i < sizeof(JudgmentValues) / sizeof(double); i++)
        judgment_time[i] = JudgmentValues[i];

    max_window_skip = SKJ_W2; /* perfect */
}

void TimingWindowsStepmania::DefaultSetup() {
    Setup(0, 0);
}

/*
 * osu!mania
 */
void TimingWindowsOsuMania::Setup(double strictness, double scale) {
    const auto od = strictness;
    // w1, w2, w3, w4, w5, miss
    static constexpr double JudgmentValues[] = {16, 16, 64, 97, 127, 151, 188};

    judgment_time[SKJ_W0] = -1; /* disable */
    judgment_time[SKJ_W1] = JudgmentValues[SKJ_W1];
    for (int i = SKJ_W2; i < sizeof(JudgmentValues) / sizeof(double); i++)
        judgment_time[i] = JudgmentValues[i] - od * 3;

    late_miss_threshold = judgment_time[SKJ_W5]; // No w0 so...
    early_hit_threshold = judgment_time[SKJ_MISS];
    early_miss_threshold = early_hit_threshold;

    /* never use w0 - enforce always using w1 through w5.  */
    current_window_skip = 1;
    min_window_skip = 1;
    max_window_skip = 1;
    combo_leniency = SKJ_W5;

    lane_hold_delta_time.fill(-1);
}

ScoreKeeperJudgment TimingWindowsOsuMania::GetJudgmentForTimeOffset(double time_delta, uint32_t lane, NoteJudgmentPart part) {
    if (part == NoteJudgmentPart::NOTE)
        return TimingWindows::GetJudgmentForTimeOffset(time_delta, lane, part);
    if (part == NoteJudgmentPart::HOLD_HEAD) {
        lane_hold_delta_time[lane] = abs(time_delta); // NOLINT(cppcoreguidelines-narrowing-conversions)
        return SKJ_NONE; // no judgments until the tail
    }

    // we're in the tail! now comes the funny part
    if (lane_hold_delta_time[lane] < 0) // guard against unlikely possibility or abuse
        return SKJ_NONE;


    const double diff_start = lane_hold_delta_time[lane];
    const double diff_end = abs(time_delta);
    const double diff_total = diff_start + diff_end;

    lane_hold_delta_time[lane] = -1; // disable

    if (time_delta < -judgment_time[SKJ_W5]) // early release
        return SKJ_MISS;

    // don't ask where I got this
    ScoreKeeperJudgment skj;
    if (diff_start <= judgment_time[SKJ_W1] * 1.2 && diff_total <= judgment_time[SKJ_W1] * 2.4)
        skj = SKJ_W1;
    else if (diff_start <= judgment_time[SKJ_W2] * 1.1 && diff_total <= judgment_time[SKJ_W2] * 2.2)
        skj = SKJ_W2;
    else if (diff_start <= judgment_time[SKJ_W3] && diff_total <= judgment_time[SKJ_W3] * 2.0)
        skj = SKJ_W3;
    else if (diff_start <= judgment_time[SKJ_W4] && diff_total <= judgment_time[SKJ_W4] * 2.0)
        skj = SKJ_W4;
    else
        skj = SKJ_W5;

    return skj;
}

TimingWindowsOsuMania::TimingWindowsOsuMania(bool _score_v2) :
    score_v2(_score_v2) {
}

void TimingWindowsOsuMania::DefaultSetup() {
    Setup(8, 1);
}

void TimingWindowsOsuMania::Reset() {
    TimingWindows::Reset();
    lane_hold_delta_time.fill(-1);
}

double TimingWindowsOsuMania::GetTickInterval() {
    return 0.1;
}

void TimingWindowsOsuMania::AddJudgment(ScoreKeeperJudgment skj, bool early_miss) {
    if (skj == SKJ_TICK) {
        combo++;
        return;
    }

    TimingWindows::AddJudgment(skj, false);
}

bool TimingWindowsOsuMania::UsesTwoJudgesPerHold() const {
    return false;
}

/*
 * O2Jam
 */

void TimingWindowsO2Jam::Setup(double strictness, double scale) {
    // This in beats.
    static constexpr double o2jamTimingAmt[] =
            {
                    O2_WINDOW * 0.1, // raindrop!o2jam extension: XCOOL
                    O2_WINDOW * 0.2, // COOL threshold
                    O2_WINDOW * 0.5, // GOOD threshold
                    O2_WINDOW * 0.8, //  BAD threshold
                    O2_WINDOW // MISS threshold
            };

    judgment_time[SKJ_W0] = o2jamTimingAmt[0];
    judgment_time[SKJ_W1] = o2jamTimingAmt[1];
    judgment_time[SKJ_W2] = o2jamTimingAmt[2];
    judgment_time[SKJ_W3] = o2jamTimingAmt[3];
    judgment_time[SKJ_W4] = -1;
    judgment_time[SKJ_W5] = -1;
    judgment_time[SKJ_MISS] = o2jamTimingAmt[4];


    // No early misses, only plain misses.
    late_miss_threshold = o2jamTimingAmt[3];
    early_miss_threshold = o2jamTimingAmt[3]; // "non-existent"
    early_hit_threshold = o2jamTimingAmt[4];

    current_window_skip = 1;
    max_window_skip = 1;
    combo_leniency = SKJ_W2;
}

void TimingWindowsO2Jam::DefaultSetup() {
    Setup(0, 0);
}

/*
 * LR2Oraja
 * */
void TimingWindowsLR2Oraja::DefaultSetup() {
    Setup(0, 100);
}

void TimingWindowsLR2Oraja::Setup(double strictness, double scale) {
    static constexpr double wnd_note[] =  { -1 , 21  , 60  , 120 , 200 , -1 , 1000 } ;
    static constexpr double wnd_ln[]   =  { -1 , 120 , 160 , 170 , 200 , -1 , 1000 } ;


    // straight up copy windows first
    memcpy(judgment_time.data(), wnd_note, judgment_time.size() * sizeof (double));
    memcpy(judgeLN.data(), wnd_ln, judgeLN.size() * sizeof (double));

    // interpolate
    ScaleByDefExRank(judgment_time, scale);
    ScaleByDefExRank(judgeLN, scale);

    early_hit_threshold = wnd_note[SKJ_W4]; /* this is the same for both LN and regular note kinds */
    early_miss_threshold = wnd_note[SKJ_MISS];
    late_miss_threshold = wnd_note[SKJ_W4];

    current_window_skip = 1;
    min_window_skip = 1;
    max_window_skip = 1;
}

void TimingWindowsLR2Oraja::ScaleByDefExRank(std::array<double, JUDGMENT_ARRAY_SIZE> &in_out, double scale) {
    static constexpr double wnd_scale[3][5] = {
           { 0 , 8  , 15 , 18  , 21  }  ,     // W1            // PG: DefEx 0    = > 0 , DefEx 100 = > 21
           { 0 , 24 , 30 , 40  , 60  }  ,     // W2            // Great: DefEx 0 = > 0 , DefEx 100 = > 60
           { 0 , 40 , 60 , 100 , 120 }        // W3            // Good: DefEx 0 =  > 0 , DefEx 100 = > 120

    };

    if (scale < 100) {
        int judge_index = scale / 25; // every 25 judges it changes difficulty (VHARD, HARD, NORMAL, etc..)
        auto interpolate = fmod(scale, 25.0);

        for (int skj = SKJ_W1; skj < SKJ_W4; ++skj) {
            auto judge = wnd_scale[skj - SKJ_W1];
            auto delta = judge[judge_index + 1] - judge[judge_index] + 12;
            auto interJudge = judge[judge_index] + interpolate * delta / 25.0;
            in_out[skj] = in_out[skj] * interJudge / judge[4];
        }
    } else {
        for (int skj = SKJ_W1; skj < SKJ_W4; ++skj) {
            in_out[skj] = in_out[skj] * scale / 100.0;
        }
    }

    // exceeding the bad windows
    for (int i = SKJ_W3; i >= SKJ_W1; --i) {
        if (in_out[i] > in_out[i + 1])
            in_out[i] = in_out[i + 1];
    }
}

ScoreKeeperJudgment TimingWindowsLR2Oraja::GetJudgmentForTimeOffset(double time_delta, uint32_t lane, NoteJudgmentPart part) {
    if (part == NoteJudgmentPart::NOTE)
        return TimingWindows::GetJudgmentForTimeOffset(time_delta, lane, part);
    if (part == NoteJudgmentPart::HOLD_HEAD)
    {
        hold_head_delta[lane] = time_delta; // NOLINT(cppcoreguidelines-narrowing-conversions)
        return SKJ_NONE;
    }

    if (part == NoteJudgmentPart::HOLD_TAIL) {
        if (hold_head_delta[lane] < 0) // head hasn't been hit or missed so don't really care.
            return SKJ_NONE;

        /* early release or late stayed-pressed */
        if (time_delta < -early_hit_threshold) {
            return SKJ_W4; /* https://github.com/wcko87/lr2oraja/commit/afddfba93357d2a73ddc94b803cf41ac5c6e8ab5 */
        }

        if (time_delta > late_miss_threshold) {
            return SKJ_MISS;
        }

        /* in judgment windows press */
        return GetJudgmentFromTimingWindows(judgeLN, hold_head_delta[lane]);
    }
}

void TimingWindowsLR2Oraja::Reset() {
    TimingWindows::Reset();
    hold_head_delta.fill(-1);
}

bool TimingWindowsLR2Oraja::UsesTwoJudgesPerHold() const {
    return false;
}
