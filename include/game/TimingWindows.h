#pragma once

#include <game/GameConstants.h>
#include <cstdint>
#include <array>

namespace rd {
    enum class NoteJudgmentPart {
        NOTE,
        HOLD_HEAD,
        HOLD_TAIL
    };

    class TimingWindows {
    protected:
        static constexpr auto JUDGMENT_ARRAY_SIZE = 7; /* w0-w5 + miss */
        /* all of these variables need to be set up by all inheriting classes. */

        /* used by the default impl. of GetJudgmentForTimeOffset.
         * Should be sorted from W0 to Wwhatever.
         * negative values disable a timing window because
         * we always compare the absolute delta_time to judgment_time. */
        std::array<double, JUDGMENT_ARRAY_SIZE> judgment_time_;
        std::array<uint32_t, JUDGMENT_ARRAY_SIZE> judgment_amt_; /* keeps track of judgments */

        uint32_t combo_ = 0;
        uint32_t max_combo_ = 0;
        uint32_t notes_hit_ = 0;
        ScoreKeeperJudgment combo_leniency_ = SKJ_W3;

        /* skip this many timing windows.
         * if 0, starts from w0. if 1, starts from w1, etc.
         * to start scanning from the [current_window_skip]th index of the judgment_time array
         * the maximum range of the window skip is set by min and max window skip.
         * it's taken into account by SetWindowSkip. */
        uint32_t current_window_skip_ = 0;
        uint32_t min_window_skip_ = 0;
        uint32_t max_window_skip_ = 1;

        /* miss thresholds; notes hit outside here count as missess
         * units are in whatever GetTimeUnits returns.
         * the range of the timing window is always from [early_miss_threshold, late_miss_threshold]
         * */
        double late_miss_threshold_; /* after time_delta > this, it's a late miss. */
        double early_miss_threshold_; /* if time_delta is > this, it's an early miss, given it's not an early hit. */
        double early_hit_threshold_;  /* if time_delta is > this, it's inside the timing windows. */

        ScoreKeeperJudgment get_judgment_from_timing_windows(const std::array<double, JUDGMENT_ARRAY_SIZE>& windows, double time_delta) const;

    public:
        virtual ~TimingWindows() = default;

        TimingWindows();

        /* strictness in whatever scale is appropriate (OD, judge rank...)
         * implementations are not required to actually respond to strictness nor scale.
         * */
        virtual void default_setup();
        virtual void setup(double strictness, double scale);
        virtual void reset();

        /* all should respond the same if it's out of range: SKJ_NONE.
        * time_delta convention: negative: early, positive: late
         * some timing systems respond differently to different parts of the note (note, head, tail)
         * so inform them of what we're judging so they do whatever adjustments they need.
         * osu!mania even needs to keep state of the total of the time_deltas of head and tail,
         * so that's per-lane information.
         * */
        virtual ScoreKeeperJudgment get_judgment_for_time_offset(double time_delta, uint32_t lane, NoteJudgmentPart part);
        virtual void update_combo(ScoreKeeperJudgment skj, bool should_break_combo);
        double get_early_threshold() const; /* in the units described by GetTimeUnits */
        double get_late_threshold() const; /* in the units described by GetTimeUnits */

        /* judgment counts */
        virtual void add_judgment(ScoreKeeperJudgment skj, bool early_miss);
        uint32_t get_judgment_count(ScoreKeeperJudgment skj) const;
        double get_judgment_window(ScoreKeeperJudgment skj);
        uint32_t get_combo() const;
        uint32_t get_max_combo() const;

        /* time units: TT_TIME works exclusively in MS, unlike the rest of the code. */
        virtual TimingType get_time_units();

        // set how many timing windows to skip (from w0)
        void SetWindowSkip(uint32_t window_skip);
        uint32_t get_window_skip() const;

        double get_early_hit_cutoff() const;

        uint32_t get_notes_hit() const;

        /* if we generate ticks for health bars, return how long to wait between them in seconds. */
        virtual double get_tick_interval();

        // true by default, unless overridden
        virtual bool uses_two_judges_per_hold() const; // if false, uses only one judgment per hold
    };
}