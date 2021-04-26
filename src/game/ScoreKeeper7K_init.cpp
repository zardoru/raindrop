#include "rmath.h"

#include <game/GameConstants.h>
#include <game/ScoreKeeper7K.h>
#include <cmath>

namespace rd {

    void ScoreKeeper::init() {
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

        total_score_objects = 0;
        total_holds = 0;
        judged_notes = 0;

        total_sqdev = 0;
        accuracy = 0;

        Scores = {
                {ST_LR2, &score_lr2},
                {ST_EX, &score_ex},
                {ST_O2JAM, &score_o2jam},
                {ST_OSUMANIA, &score_osumania},
                {ST_OSUMANIA_ACC, &score_osumania_acc},
                {ST_RANK, &score_rank},
                {ST_EXP, &score_exp},
                {ST_EXP3, &score_exp3},
                {ST_IIDX, &score_bms}
        };

        for (auto &scoresys: Scores) {
            scoresys.second->Reset();
        }

        Gauges = {
                {LT_GROOVE, &gauge_groove},
                {LT_EASY, &gauge_easy},
                {LT_SURVIVAL, &gauge_survival},
                {LT_EXHARD, &gauge_exhard},
                {LT_DEATH, &gauge_death},
                {LT_STEPMANIA, &gauge_stepmania},
                {LT_O2JAM, &gauge_o2jam},
                {LT_OSUMANIA, &gauge_osumania},
                {LT_LR2_ASSIST, &gauge_lr2_assist},
                {LT_LR2_EASY, &gauge_lr2_easy},
                {LT_LR2_NORMAL, &gauge_lr2_normal},
                {LT_LR2_HARD, &gauge_lr2_hard},
                {LT_LR2_EXHARD, &gauge_lr2_exhard},
                {LT_LR2_HAZARD, &gauge_lr2_hazard},
                {LT_LR2_CLASS, &gauge_lr2_class},
                {LT_LR2_EXCLASS, &gauge_lr2_exclass},
                {LT_LR2_EXHARDCLASS, &gauge_lr2_exhardclass}
        };

        for (auto &gauge: Gauges) {
            gauge.second->DefaultSetup();
            gauge.second->Reset();
        }

        Timings = {
                {TI_RAINDROP, &timing_raindrop},
                {TI_OSUMANIA, &timing_osumania},
                {TI_O2JAM, &timing_o2jam},
                {TI_STEPMANIA, &timing_stepmania},
                {TI_LR2, &timing_lr2}
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

        double lifebar_total_lr2;
        if (total != -1 && std::isnan(multiplier) && !std::isnan(total)) {
            lifebar_total = total;
            lifebar_total_lr2 = total;
        } else {
            auto max_notes = getMaxJudgableNotes();
            lifebar_total =
                    std::max(260.0, 7.605 * max_notes / (6.5 + 0.01 * max_notes)) * effmul;

            lifebar_total_lr2 = 160.0 + (max_notes + std::min(std::max(max_notes, 390), 600) - 390) * 11.0 / 70.0;
        }

        // recalculate groove lifebar increments.
        gauge_death.Setup(lifebar_total, getMaxJudgableNotes(), 0);
        gauge_exhard.Setup(lifebar_total, getMaxJudgableNotes(), 0);
        gauge_survival.Setup(lifebar_total, getMaxJudgableNotes(), 0);
        gauge_easy.Setup(lifebar_total, getMaxJudgableNotes(), 0);
        gauge_groove.Setup(lifebar_total, getMaxJudgableNotes(), 0);
        gauge_lr2_assist.Setup(lifebar_total_lr2, getMaxJudgableNotes(), 0);
        gauge_lr2_easy.Setup(lifebar_total_lr2, getMaxJudgableNotes(), 0);
        gauge_lr2_normal.Setup(lifebar_total_lr2, getMaxJudgableNotes(), 0);
        gauge_lr2_hard.Setup(lifebar_total_lr2, getMaxJudgableNotes(), 0);
        gauge_lr2_exhard.Setup(lifebar_total_lr2, getMaxJudgableNotes(), 0);
        gauge_lr2_hazard.Setup(lifebar_total_lr2, getMaxJudgableNotes(), 0);
        gauge_lr2_class.Setup(lifebar_total_lr2, getMaxJudgableNotes(), 0);
        gauge_lr2_exclass.Setup(lifebar_total_lr2, getMaxJudgableNotes(), 0);
        gauge_lr2_exhardclass.Setup(lifebar_total_lr2, getMaxJudgableNotes(), 0);
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
        double lr2_rank = 75;
        switch (rank) {
            case 0: // VHARD
                judge_window_scale = 1;
                lr2_rank = 25;
                break;
            case 1: // HARD
                judge_window_scale = 1.2;
                lr2_rank = 50;
                break;
            case 2: // NORMAL
                judge_window_scale = 1.4;
                lr2_rank = 75;
                break;
            case 3: // EASY
                judge_window_scale = 1.7;
                lr2_rank = 100;
                break;
            case 4: // VEASY
                judge_window_scale = 2;
                break;
            default:
                break;
        }

        timing_raindrop.Setup(0, judge_window_scale);
        timing_lr2.Setup(0, lr2_rank);
        setBMSTimingWindows();
    }

    void ScoreKeeper::setJudgeScale(double scale) {
        timing_raindrop.Setup(0, scale * 100.0 / 72.0);
        timing_lr2.Setup(0, scale);
        setBMSTimingWindows();
    }
}