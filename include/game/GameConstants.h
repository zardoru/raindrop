#pragma once

#include <cstdint>

namespace rd {
    const auto DEFAULT_WAIT_TIME = 1.5;

    const int SCRATCH_1P_CHANNEL = 0;
    const int SCRATCH_2P_CHANNEL = 8;

    inline bool IsScratchLane(uint32_t lane) {
        return lane == SCRATCH_1P_CHANNEL || lane == SCRATCH_2P_CHANNEL;
    }

    // note type: 3 bits
    enum ENoteKind
    {
        NK_NORMAL,
        NK_FAKE,
        NK_MINE,
        NK_LIFT,
        NK_ROLL, // subtype of hold
        NK_INVISIBLE,
        NK_TOTAL
    };

    enum ChartType
    {
        // Autodecide - Not a value for anything other than PlayscreenParameters!
        TI_NONE = 0,
        TI_BMS = 1,
        TI_OSUMANIA = 2,
        TI_O2JAM = 3,
        TI_STEPMANIA = 4,
        TI_RAINDROP = 5,
        TI_RDAC = 6,
        TI_LR2 = 7
    };

    const uint8_t MAX_CHANNELS = 128; // full MIDI range

    enum ScoreKeeperJudgment {
        SKJ_NONE = -1, // no judgment.

        SKJ_W0 = 0, // Ridiculous / rainbow 300, for those that use it.

        SKJ_W1 = 1, // J_PERFECT / flashing J_GREAT
        SKJ_W2 = 2, // J_GREAT
        SKJ_W3 = 3, // Good
        SKJ_W4 = 4, // Bad
        SKJ_W5 = 5, // W5 is unused in beatmania.

        SKJ_MISS = 6, // Miss / Poor

        SKJ_HOLD_OK = 10, // OK, only used with DDR-style holds
        SKJ_HOLD_NG = 11, // NG
        SKJ_MINE = 12, // whoops you pressed a mine
        SKJ_TICK = 13 // long note makes combo go brrrrrrrrr
    };

    enum ScoreType
    {
        ST_RANK = 1, // raindrop rank scoring

        ST_EX = 2, // EX score
        ST_DP = 3, // DP score

        ST_IIDX = 10, // IIDX score
        ST_LR2 = 11, // LR2 score

        ST_SCORE = 20, // score out of 100m
        ST_OSUMANIA = 21, // osu!mania scoring
        ST_JB2 = 22, // jubeat^2 scoring

        ST_EXP = 30, // experimental scoring.
        ST_EXP3 = 31, // experimental scoring.

        ST_O2JAM = 40, // O2jam scoring.

        ST_COMBO = 100, // current combo
        ST_MAX_COMBO = 101, // max combo
        ST_NOTES_HIT = 102, // total notes hit
    };

    enum PercentScoreType
    {
        PST_RANK = 1, // raindrop rank score

        PST_EX = 2, // EX score
        PST_NH = 3, // % notes hit
        PST_ACC = 4, // Accuracy

        PST_OSU = 5, // osu!mania
    };

    const double SCORE_MAX = 100000000;

    enum LifeType {

        LT_AUTO = 0, // Only for PlayscreenParameters
        // actual groove type should be set by playing field from chart

        LT_GROOVE = 1, // Beatmania default lifebar
        LT_SURVIVAL = 2, // Beatmania hard mode
        LT_EXHARD = 3, // Beatmania EX hard mode
        LT_DEATH = 4, // Sudden death mode

        LT_EASY = 5, // Beatmania easy mode

        LT_STEPMANIA = 6, // Stepmania/DDR/osu!mania scoring mode.
        LT_NORECOV = 7, // DDR no recov. mode
        LT_O2JAM = 8,
        LT_OSUMANIA = 9,

        LT_BATTERY = 11, // DDR battery mode.
    };

    // Pacemakers.
    enum PacemakerType
    {
        PT_NONE = 0,

        PMT_A = 1,
        PMT_AA = 2,
        PMT_AAA = 3,

        PMT_50EX = 4,
        PMT_75EX = 5,
        PMT_85EX = 6,
        PMT_90EX = 7,

        PMT_RANK_ZERO = 20,
        PMT_RANK_P1 = 21,
        PMT_RANK_P2 = 22,
        PMT_RANK_P3 = 23,
        PMT_RANK_P4 = 24,
        PMT_RANK_P5 = 25,
        PMT_RANK_P6 = 26,
        PMT_RANK_P7 = 27,
        PMT_RANK_P8 = 28,
        PMT_RANK_P9 = 29,

        PMT_B = 31,
        PMT_C = 32,
        PMT_D = 33,
        PMT_E = 34,
        PMT_F = 35,
    };

    enum TimingType
    {
        TT_TIME,
        TT_BEATS,
        TT_PIXELS
    };

    constexpr double O2_WINDOW = 0.664;

    /* vsrg constants */

    enum ESpeedType
    {
        SPEEDTYPE_DEFAULT = -1,
        SPEEDTYPE_FIRST, // first, after SV
        SPEEDTYPE_MMOD, // maximum mod
        SPEEDTYPE_CMOD, // constant
        SPEEDTYPE_FIRSTBPM, // first, ignoring sv
        SPEEDTYPE_MODE, // most common
        SPEEDTYPE_MULTIPLIER // use target speed as multiplier directly
    };
}


