#include "rmath.h"

#include <game/GameConstants.h>
#include <game/ScoreKeeper7K.h>
#include <cmath>

namespace rd {

    void ScoreKeeper::init() {
        use_w0_for_ex2 = false;

        rank_pts = 0;
        use_o2jam = false;
        avg_hit = 0;
        hit_variance = 0;

        pacemaker_texts[PMT_F] = "F";
        pacemaker_texts[PMT_E] = "E";
        pacemaker_texts[PMT_D] = "D";
        pacemaker_texts[PMT_C] = "C";
        pacemaker_texts[PMT_B] = "B";
        pacemaker_texts[PMT_A] = "A";
        pacemaker_texts[PMT_AA] = "AA";
        pacemaker_texts[PMT_AAA] = "AAA";

        pacemaker_texts[PMT_RANK_ZERO] = "0";
        pacemaker_texts[PMT_RANK_P1] = "+1";
        pacemaker_texts[PMT_RANK_P2] = "+2";
        pacemaker_texts[PMT_RANK_P3] = "+3";
        pacemaker_texts[PMT_RANK_P4] = "+4";
        pacemaker_texts[PMT_RANK_P5] = "+5";
        pacemaker_texts[PMT_RANK_P6] = "+6";
        pacemaker_texts[PMT_RANK_P7] = "+7";
        pacemaker_texts[PMT_RANK_P8] = "+8";
        pacemaker_texts[PMT_RANK_P9] = "+9";

        setAccMin(6.4);
        setAccMax(100);

        max_notes = 0;

        score = 0;

        rank_w0_count = 0;
        rank_w1_count = 0;
        rank_w2_count = 0;
        rank_w3_count = 0;

        total_notes = 0;

        ex_score = 0;

        bms_combo = 0;
        bms_combo_pts = 0;
        bms_dance_pts = 0;
        bms_score = 0;

        lr2_dance_pts = 0;
        lr2_score = 0;

        osu_points = 0;
        bonus_counter = 100;
        osu_bonus_points = 0;

        osu_score = 0;
        osu_accuracy = 100;

        exp_combo = 0;
        exp_combo_pts = 0;
        exp_max_combo_pts = 0;
        exp_hit_score = 0;
        exp_score = 0;
        exp3_score = 0;

        sc_score = 0;
        sc_sc_score = 0;

        coolcombo = 0;
        pills = 0;
        jams = 0;
        jam_jchain = 0;
        o2_score = 0;

        total_sqdev = 0;
        accuracy = 0;

        Gauges = {
                {LT_GROOVE, &gauge_groove},
                {LT_EASY, &gauge_easy},
                {LT_SURVIVAL, &gauge_survival},
                {LT_EXHARD, &gauge_exhard},
                {LT_DEATH, &gauge_death},
                {LT_STEPMANIA, &gauge_stepmania},
                {LT_O2JAM, &gauge_o2jam}
        };

        for (auto &gauge: Gauges) {
            gauge.second->DefaultSetup();
            gauge.second->Reset();
        }

        Timings = {
                {TI_RAINDROP, &timing_raindrop},
                {TI_OSUMANIA, &timing_osumania},
                {TI_O2JAM, &timing_o2jam},
                {TI_STEPMANIA, &timing_stepmania}
        };

        for (auto &timing: Timings) {
            timing.second->DefaultSetup();
            timing.second->Reset();
        }

        CurrentTimingWindow = &timing_raindrop;

        for (auto i = -127; i < 128; ++i)
            histogram[i + 127] = 0;
    }

    void ScoreKeeper::setO2JamBeatTimingWindows() {
        CurrentTimingWindow = &timing_o2jam;
    }

    void ScoreKeeper::setBMSTimingWindows() {
        CurrentTimingWindow = &timing_raindrop;
    }

    void ScoreKeeper::setODWindows(int od) {
        timing_osumania.Setup(od, 1);
        CurrentTimingWindow = &timing_osumania;
    }

    void ScoreKeeper::setUseW0(bool on) {
        for (auto &timing: Timings) {
            if (on)
                timing.second->SetWindowSkip(1);
            else
                timing.second->SetWindowSkip(0);

        }
    } // make a config option

    ScoreKeeper::ScoreKeeper() {
        init();
    }

    ScoreKeeper::ScoreKeeper(double judge_window_scale) {
        init();
        timing_raindrop.Setup(0, judge_window_scale);
        setBMSTimingWindows();
    }

    void ScoreKeeper::setSMJ4Windows() {
        CurrentTimingWindow = &timing_stepmania;
    }

    void ScoreKeeper::setLifeTotal(double total, double multiplier) {
        double effmul = std::isnan(multiplier) ? 1 : multiplier;

        if (total != -1 && std::isnan(multiplier) && !std::isnan(total))
            lifebar_total = total;
        else
            lifebar_total = std::max(260.0, 7.605 * max_notes / (6.5 + 0.01 * max_notes)) * effmul;

        // recalculate groove lifebar increments.
        gauge_death.Setup(lifebar_total, max_notes, 0);
        gauge_exhard.Setup(lifebar_total, max_notes, 0);
        gauge_survival.Setup(lifebar_total, max_notes, 0);
        gauge_easy.Setup(lifebar_total, max_notes, 0);
        gauge_groove.Setup(lifebar_total, max_notes, 0);
    }

    void ScoreKeeper::setO2LifebarRating(int difficulty) {
        gauge_o2jam.Setup(0, 0, difficulty);
    }

   void ScoreKeeper::setJudgeRank(int rank) {

        if (rank == -100) // We assume we're dealing with beats-based timing.
        {
            use_o2jam = true;
            setO2JamBeatTimingWindows();
            return;
        }

        use_o2jam = false;

        // old values: 0.5, 0.75, 1.0, 1.5, 2.0
        /*switch (rank) { // second version
        case 0:
            judge_window_scale = 8.0 / 11.0; break;
        case 1:
            judge_window_scale = 9.0 / 11.0; break;
        case 2:
            judge_window_scale = 1.00; break;
        case 3:
            judge_window_scale = 13.0 / 11.0; break;
        case 4:
            judge_window_scale = 16.0 / 11.0; break;
        }*/

        // third version
        float judge_window_scale = 1.7;
        switch (rank) {
            case 0:
                judge_window_scale = 1;
                break;
            case 1:
                judge_window_scale = 1.2;
                break;
            case 2:
                judge_window_scale = 1.4;
                break;
            case 3:
                judge_window_scale = 1.7;
                break;
            case 4:
                judge_window_scale = 2;
                break;
            default:
                break;
        }

       timing_raindrop.Setup(0, judge_window_scale);
        setBMSTimingWindows();
    }

    void ScoreKeeper::setJudgeScale(double scale) {
        timing_raindrop.Setup(0, scale * 100.0 / 72.0);
        setBMSTimingWindows();
    }
}