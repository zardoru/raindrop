#pragma once

#include <unordered_map>
#include <game/gauges/GaugeBMS.h>
#include <game/gauges/GaugeStepmania.h>
#include <game/gauges/GaugeO2Jam.h>
#include <game/gauges/GaugeLR2Oraja.h>

#include <game/timing_windows/TimingWindowsO2Jam.h>
#include <game/timing_windows/TimingWindowsOsuMania.h>
#include <game/timing_windows/TimingWindowsRaindropBMS.h>
#include <game/timing_windows/TimingWindowsStepmania.h>
#include <game/timing_windows/TimingWindowsLR2Oraja.h>
#include <game/gauges/GaugeOsuMania.h>

#include <game/scoring_systems/RaindropScoring.h>
#include <game/scoring_systems/O2JamScoring.h>
#include <game/scoring_systems/BMSScoring.h>
#include <game/scoring_systems/OsumaniaScoring.h>

namespace rd {
    class ScoreKeeper {

    public:

        ScoreKeeper();

        ScoreKeeper(double judge_window_scale);

        ~ScoreKeeper();

        void init();

        void setTotalObjects(int total_objects, int total_holds);

        // total if multiplier is nan, else default rate * multiplier - has to be called after setting
        // any timing parameters like judge rank or OD and after setting total objects.
        void setLifeTotal(double total, double multiplier = NAN);

        void setO2LifebarRating(int difficulty);

        void setJudgeRank(int rank);

        void setJudgeScale(double scale);

        void setODWindows(int od);

        void setSMJ4Windows();

        void setAccMin(double ms);

        void setAccMax(double ms);

        // accessor functions

        int getMaxJudgableNotes() const;

        int getJudgedNotes() const;

        int getJudgmentCount(int Judge) const;

        std::string getHistogram();

        int getHistogramPoint(int point) const;

        int getHistogramPointCount() const;

        int getHistogramHighestPoint() const;

        double getAvgHit() const;

        ScoreKeeperJudgment hitNote(double ms, uint32_t lane, NoteJudgmentPart part);

        void lifebarHit(double ms, rd::ScoreKeeperJudgment judgment);

        void missNote(bool dont_break_combo, bool early_miss, bool apply_miss);

        double getAccMax() const;

        double getJudgmentWindow(int judgment);

        double getLateMissCutoffMS() const;

        double getEarlyMissCutoffMS() const;

        double getEarlyHitCutoffMS() const;

        double getJudgmentCutoffMS();

        int getScore(int score_type) const;

        float getPercentScore(int score_type) const;

        float getLifebarAmount(int lifebar_amount_type) const;

        bool isStageFailed(int lifebar_amount_type) const;

        bool hasDelayedFailure(int lifebar_type);

        void failStage();

        int getPacemakerDiff(PacemakerType pmt);

        std::pair<std::string, int> getAutoPacemaker();

        std::pair<std::string, int> getAutoRankPacemaker();

        std::unordered_map<PacemakerType, std::string> pacemaker_texts;

        void applyRateScale(double rate);


        int getRank() const; // returns a number from -9 to 9
        int getBMRank() const; // returns PMT_xxx according to EXScore Rank

        uint8_t getPills() const;

        int getCoolCombo() const;

        void setUseW0(bool);

        bool usesW0() const;

        bool usesO2() const;

        float getHitStDev() const;

        // percent we're sure the offset is wrong
        double getOffsetDistrust() const;

        void useLR2Timing();

        void setOsuHP(float hp);

        double getLNTickInterval();

        void tickLN(int ticks);

    private:

        void setO2JamBeatTimingWindows();

        // online avg hit and variance
        double avg_hit;
        double hit_variance;

        // o2jam-specific variable
        bool use_o2jam;

        /*
            Standard scoring.
        */

        double score; // standard score.
        double sc_score;
        double sc_sc_score;

        long long total_score_objects;
        long long total_holds;

        /*
         * Score Systems
         */

        // BMS
        ScoreSystemBMS score_bms;
        ScoreSystemEX score_ex;
        ScoreSystemLR2 score_lr2;

        // O2Jam
        ScoreSystemO2Jam score_o2jam;

        // osu!mania
        ScoreSystemOsuMania score_osumania;
        ScoreSystemOsuManiaAccuracy score_osumania_acc;

        // Raindrop
        ScoreSystemExp score_exp;
        ScoreSystemExp3 score_exp3;
        ScoreSystemRank score_rank;

        std::unordered_map<ScoreType, ScoringSystem*> Scores;

        /*
            misc.
        */

        long long judged_notes;


        long long dp_score; // DDR dance-point scoring
        long long dp_dp_score;


        double total_sqdev; // accuracy scoring
        double accuracy;

        double accuracy_percent(double var);

        // lifebar data.

        double lifebar_total;
        GaugeGroove gauge_groove;
        GaugeEasy gauge_easy;
        GaugeSurvival gauge_survival;
        GaugeExHard gauge_exhard;
        GaugeDeath gauge_death;
        GaugeStepmania gauge_stepmania;
        GaugeO2Jam gauge_o2jam;
        GaugeOsuMania gauge_osumania;

        GaugeLR2Assist gauge_lr2_assist;
        GaugeLR2Easy gauge_lr2_easy;
        GaugeLR2Normal gauge_lr2_normal;
        GaugeLR2Hard gauge_lr2_hard;
        GaugeLR2ExHard gauge_lr2_exhard;
        GaugeLr2Hazard gauge_lr2_hazard;
        GaugeLr2Class gauge_lr2_class;
        GaugeLr2ExClass gauge_lr2_exclass;
        GaugeLr2ExHardClass gauge_lr2_exhardclass;

        std::unordered_map<LifeType, Gauge*> Gauges;

        // judgment information
        TimingWindowsO2Jam timing_o2jam;
        TimingWindowsOsuMania timing_osumania;
        TimingWindowsRaindropBMS timing_raindrop;
        TimingWindowsStepmania timing_stepmania;
        TimingWindowsLR2Oraja timing_lr2;

        std::unordered_map<ChartType, TimingWindows*> Timings;
        TimingWindows* CurrentTimingWindow;

        void setBMSTimingWindows();

        double histogram[255]; // records from -127 to +127 ms.

        // no-recovery modes.
        double lifebar_battery;

        long long lifebar_battery_lives;

        // scoring parameters.
        double ACC_MIN, ACC_MIN_SQ;
        double ACC_MAX, ACC_MAX_SQ;

    };

    void SetupScorekeeperLuaInterface(void *state);

    void SetScorekeeperInstance(void *state, ScoreKeeper *Instance);
}



