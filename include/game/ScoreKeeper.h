#pragma once

#include <cmath>
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

        void set_total_objects(int total_objects, int total_holds);

        // total if multiplier is nan, else default rate * multiplier - has to be called after setting
        // any timing parameters like judge rank or OD and after setting total objects.
        void set_life_total(double total, double multiplier = NAN);

        void set_o2_lifebar_rating(int difficulty);

        void set_judge_rank(int rank);

        void set_judge_scale(double scale);

        void set_od_windows(int od);

        void set_smj4_windows();

        void set_acc_min(double ms);

        void set_acc_max(double ms);

        // accessor functions

        int get_max_judgable_notes() const;

        int get_judged_notes() const;

        int get_judgment_count(int Judge) const;

        std::string get_histogram();

        int get_histogram_point(int point) const;

        int get_histogram_point_count() const;

        int get_histogram_highest_point() const;

        double get_avg_hit() const;

        ScoreKeeperJudgment hit_note(double ms, uint32_t lane, NoteJudgmentPart part);

        void lifebar_hit(double ms, rd::ScoreKeeperJudgment judgment);

        void miss_note(bool dont_break_combo, bool early_miss, bool apply_miss);

        double get_acc_max() const;

        double get_judgment_window(int judgment);

        double get_late_miss_cutoff_ms() const;

        double get_early_miss_cutoff_ms() const;

        double get_early_hit_cutoff_ms() const;

        double get_judgment_cutoff_ms();

        int get_score(int score_type) const;

        float get_percent_score(int score_type) const;

        float get_lifebar_amount(int lifebar_amount_type) const;

        bool is_stage_failed(int lifebar_amount_type) const;

        bool has_delayed_failure(int lifebar_type);

        void fail_stage();

        int get_pacemaker_diff(PacemakerType pmt);

        std::pair<std::string, int> get_auto_pacemaker();

        std::pair<std::string, int> get_auto_rank_pacemaker();

        std::unordered_map<PacemakerType, std::string> pacemaker_texts;

        void apply_rate_scale(double rate);


        int get_rank() const; // returns a number from -9 to 9
        int get_bm_rank() const; // returns PMT_xxx according to EXScore Rank

        uint8_t get_pills() const;

        int get_cool_combo() const;

        void set_use_w0(bool);

        bool uses_w0() const;

        bool is_o2jam() const;

        float get_hit_stdev() const;

        // percent we're sure the offset is wrong
        double get_offset_distrust() const;

        void use_lr2_timing();

        void set_osu_hp(float hp);

        double get_ln_tick_interval();

        void tick_ln(int ticks);

    private:

        void set_o2jam_beat_timing_windows();

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

        std::unordered_map<ScoreType, ScoringSystem*> scores;

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

        std::unordered_map<LifeType, Gauge*> gauges;

        // judgment information
        TimingWindowsO2Jam timing_o2jam;
        TimingWindowsOsuMania timing_osumania;
        TimingWindowsRaindropBMS timing_raindrop;
        TimingWindowsStepmania timing_stepmania;
        TimingWindowsLR2Oraja timing_lr2;

        std::unordered_map<ChartType, TimingWindows*> timings;
        TimingWindows* current_timing_window;

        void set_bms_timing_windows();

        double histogram[255]; // records from -127 to +127 ms.

        // no-recovery modes.
        double lifebar_battery;

        long long lifebar_battery_lives;

        // scoring parameters.
        double acc_min, acc_min_sq;
        double acc_max, acc_max_sq;

    };

    void setup_scorekeeper_lua_interface(void *state);

    void set_scorekeeper_instance(void *state, ScoreKeeper *Instance);
}



