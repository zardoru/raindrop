#pragma once

#include <map>
#include <game/gauges/GaugeBMS.h>
#include <game/gauges/GaugeStepmania.h>
#include <game/gauges/GaugeO2Jam.h>

#include <game/timing_windows/TimingWindowsO2Jam.h>
#include <game/timing_windows/TimingWindowsOsuMania.h>
#include <game/timing_windows/TimingWindowsRaindropBMS.h>
#include <game/timing_windows/TimingWindowsStepmania.h>
#include <game/timing_windows/TimingWindowsLR2Oraja.h>


namespace rd {
    class ScoreKeeper {

    public:

        ScoreKeeper();

        ScoreKeeper(double judge_window_scale);

        ~ScoreKeeper();

        void init();

        void setMaxNotes(int notes);

        // total if multiplier is nan, else default rate * multiplier
        void setLifeTotal(double total, double multiplier = NAN);

        void setO2LifebarRating(int difficulty);

        void setJudgeRank(int rank);

        void setJudgeScale(double scale);

        void setODWindows(int od);

        void setSMJ4Windows();

        void setAccMin(double ms);

        void setAccMax(double ms);

        // accessor functions

        int getMaxNotes() const;

        int getTotalNotes() const;

        int getJudgmentCount(int Judge);

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

        int getScore(int score_type);

        float getPercentScore(int score_type) const;

        float getLifebarAmount(int lifebar_amount_type);

        bool isStageFailed(int lifebar_amount_type);

        bool hasDelayedFailure(int lifebar_type);

        void failStage();

        int getPacemakerDiff(PacemakerType pmt);

        std::pair<std::string, int> getAutoPacemaker();

        std::pair<std::string, int> getAutoRankPacemaker();

        std::map<PacemakerType, std::string> pacemaker_texts;

        void applyRateScale(double rate);


        int getRank() const; // returns a number from -9 to 9
        int getBMRank() const; // returns PMT_xxx according to EXScore Rank

        long long getRankPoints() const; // returns underlying rank points

        uint8_t getPills() const;

        int getCoolCombo() const;

        void setUseW0(bool);

        bool usesW0() const;

        bool usesO2() const;

        float getHitStDev() const;

        // percent we're sure the offset is wrong
        double getOffsetDistrust() const;

        void useLR2Timing();

    private:

        void setO2JamBeatTimingWindows();

        bool use_w0_for_ex2; // whether or not to require ridiculous for 2 EX score.

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

        /*
            Rank scoring
            */

        long long rank_w0_count;
        long long rank_w1_count;
        long long rank_w2_count;
        long long rank_w3_count;

        long long rank_pts; // rank scoring

        void update_ranks(ScoreKeeperJudgment judgment);

        long long max_notes;

        /*
            BMS scoring
            */

        long long ex_score;

        long long bms_combo;
        long long bms_combo_pts;
        long long bms_max_combo_pts;

        long long bms_dance_pts;
        long long bms_score;

        long long lr2_dance_pts;
        long long lr2_score;

        void update_bms(ScoreKeeperJudgment judgment);

        void update_lr2(ScoreKeeperJudgment judgment);

        /*
            osu!
            */

        long long osu_points;
        long long osu_accuracy;
        int bonus_counter;
        double osu_bonus_points;

        double osu_score;

        void update_osu(ScoreKeeperJudgment judgment);


        /*
            experimental
            */

        long long exp_combo;
        long long exp_combo_pts;
        long long exp_max_combo_pts;

        long long exp_hit_score;

        double exp_score;

        double exp3_score;

        void update_exp2(ScoreKeeperJudgment judgment);

        /*
            o2jam
            */

        char pills;

        long long coolcombo;
        long long o2_score;
        long long jams;
        long long jam_jchain;

        void update_o2(ScoreKeeperJudgment judgment);

        int getO2Judge(ScoreKeeperJudgment j);

        /*
            misc.
            */

        long long total_notes;


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

        std::map<LifeType, Gauge*> Gauges;


        // judgment information
        TimingWindowsO2Jam timing_o2jam;
        TimingWindowsOsuMania timing_osumania;
        TimingWindowsRaindropBMS timing_raindrop;
        TimingWindowsStepmania timing_stepmania;
        TimingWindowsLR2Oraja timing_lr2;

        std::map<ChartType, TimingWindows*> Timings;
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



