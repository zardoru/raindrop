#pragma once

#include <functional>
#include <array>
#include <game/RaindropProcessedChart.h>
#include <game/TimingWindows.h>
#include <ChartGroup.h>

namespace rd {
    class ScoreKeeper;

    class Mechanics {
    public:
        typedef std::function<void(double, LaneHandle, NoteJudgmentPart)> HitEvent;
        typedef std::function<void(double, LaneHandle, bool, bool, bool)> MissEvent;
        typedef std::function<void(KeysoundHandle)> KeysoundEvent;

    protected:

        otoworm::Chart *current_chart_;
        std::shared_ptr<ScoreKeeper> player_score_keeper_;
    public:

        bool is_late_head_miss(double t, RuntimeNote *note);

        bool in_judge_cutoff(double t, RuntimeNote *note);

        bool is_early_miss(double t, RuntimeNote *note);

        bool is_bm_bad_judge(double t, RuntimeNote *note);

        /* returns true if the note is within the timing windows. */
        bool in_head_cutoff(double t, RuntimeNote *note);

        virtual ~Mechanics() = default;

        // These HAVE to be set before anything else is called.
        std::function<bool(LaneHandle)> is_lane_key_down;
        std::function<void(LaneHandle, bool)> set_lane_holding_state;
        KeysoundEvent play_keysound;
        HitEvent notify_hit;
        MissEvent notify_miss;

        virtual void transform_notes(RaindropProcessedChart &chart_state);

        virtual void configure(otoworm::Chart *chart, std::shared_ptr<ScoreKeeper> score_keeper);

        // If returns true, don't judge any more notes.
        virtual bool on_update(double song_time, RuntimeNote *note, LaneHandle lane) = 0;

        // If returns true, don't judge any more notes.
        virtual bool on_press_lane(double song_time, RuntimeNote *note, LaneHandle lane) = 0;

        // If returns true, don't judge any more notes either.
        virtual bool on_release_lane(double song_time, RuntimeNote *note, LaneHandle lane) = 0;

        virtual bool on_scratch_up(double song_time, RuntimeNote *note, LaneHandle lane);

        virtual bool on_scratch_down(double song_time, RuntimeNote *note, LaneHandle lane);

        virtual bool on_scratch_neutral(double song_time, RuntimeNote *note, LaneHandle lane);

        virtual TimingType get_timing_kind() = 0;
    };

    class RaindropMechanics : public Mechanics {
        bool forced_release_;
        std::array<double, MAX_CHANNELS> hold_hit_time_;
    public:
        explicit RaindropMechanics(bool forced_release);

        bool on_update(double song_time, RuntimeNote *note, LaneHandle lane) override;

        bool on_press_lane(double song_time, RuntimeNote *note, LaneHandle lane) override;

        bool on_release_lane(double song_time, RuntimeNote *note, LaneHandle lane) override;

        TimingType get_timing_kind() override;
    };

    class O2JamMechanics : public Mechanics {
    public:

        bool on_update(double song_beat, RuntimeNote *note, LaneHandle lane) override;

        bool on_press_lane(double song_beat, RuntimeNote *note, LaneHandle lane) override;

        bool on_release_lane(double song_beat, RuntimeNote *note, LaneHandle lane) override;

        TimingType get_timing_kind() override;
    };

    class RaindropArcadeMechanics : public Mechanics {
    public:
        enum EScratchState {
            SCR_NEUTRAL,
            SCR_UP,
            SCR_DOWN
        };

    private:
        // Left and right (case of double?)
        EScratchState scratch_state_[2];

        int get_scratch_for_lane(LaneHandle lane);

        bool can_hit_note_head(double time, RuntimeNote *note);

        bool can_hit_note_tail(double time, RuntimeNote *note);

        void judge_scratch(double song_time, RuntimeNote *note, LaneHandle lane, EScratchState new_scratch_state,
                          EScratchState old_scratch_state);

        void perform_judgement(double song_time, RuntimeNote *note, LaneHandle lane);

    public:
        RaindropArcadeMechanics();

        ~RaindropArcadeMechanics() = default;

        // If returns true, don't judge any more notes.
        bool on_update(double song_time, RuntimeNote *note, LaneHandle lane) override;

        // If returns true, don't judge any more notes.
        bool on_press_lane(double song_time, RuntimeNote *note, LaneHandle lane) override;

        // If returns true, don't judge any more notes either.
        bool on_release_lane(double song_time, RuntimeNote *note, LaneHandle lane) override;

        bool on_scratch_up(double song_time, RuntimeNote *note, LaneHandle lane) override;

        bool on_scratch_down(double song_time, RuntimeNote *note, LaneHandle lane) override;

        // If note is null, it didn't happen while an hold was being held
        // otherwise, it happened while a hold was being held
        bool on_scratch_neutral(double song_time, RuntimeNote *note, LaneHandle lane) override;

        TimingType get_timing_kind() override;
    };
}
