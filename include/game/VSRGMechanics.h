#pragma once

#include <functional>
#include <array>
#include <game/RaindropProcessedChart.h>
#include <ChartGroup.h>

namespace rd {
    class ScoreKeeper;

    class Mechanics {
    public:
        typedef std::function<void(double, uint32_t, bool, bool)> HitEvent;
        typedef std::function<void(double, uint32_t, bool, bool, bool)> MissEvent;
        typedef std::function<void(uint32_t)> KeysoundEvent;

    protected:

        otoworm::Chart *CurrentChart;
        std::shared_ptr<ScoreKeeper> PlayerScoreKeeper;
    public:

        bool IsLateHeadMiss(double t, RuntimeNote *note);

        bool InJudgeCutoff(double t, RuntimeNote *note);

        bool IsEarlyMiss(double t, RuntimeNote *note);

        bool IsBmBadJudge(double t, RuntimeNote *note);

        /* returns true if the note is within the timing windows. */
        bool InHeadCutoff(double t, RuntimeNote *note);

        virtual ~Mechanics() = default;

        // These HAVE to be set before anything else is called.
        std::function<bool(uint32_t)> is_lane_key_down;
        std::function<void(uint32_t, bool)> set_lane_holding_state;
        KeysoundEvent play_keysound;
        HitEvent notify_hit;
        MissEvent notify_miss;

        virtual void transform_notes(RaindropProcessedChart &ChartState);

        virtual void configure(otoworm::Chart *chart, std::shared_ptr<ScoreKeeper> scoreKeeper);

        // If returns true, don't judge any more notes.
        virtual bool OnUpdate(double SongTime, RuntimeNote *Note, uint32_t Lane) = 0;

        // If returns true, don't judge any more notes.
        virtual bool OnPressLane(double SongTime, RuntimeNote *Note, uint32_t Lane) = 0;

        // If returns true, don't judge any more notes either.
        virtual bool OnReleaseLane(double SongTime, RuntimeNote *Note, uint32_t Lane) = 0;

        virtual bool OnScratchUp(double SongTime, RuntimeNote *Note, uint32_t Lane);

        virtual bool OnScratchDown(double SongTime, RuntimeNote *Note, uint32_t Lane);

        virtual bool OnScratchNeutral(double SongTime, RuntimeNote *Note, uint32_t Lane);

        virtual TimingType GetTimingKind() = 0;
    };

    class RaindropMechanics : public Mechanics {
        bool forcedRelease;
        std::array<double, MAX_CHANNELS> hold_hit_time;
    public:
        explicit RaindropMechanics(bool forcedRelease);

        bool OnUpdate(double SongTime, RuntimeNote *Note, uint32_t Lane) override;

        bool OnPressLane(double SongTime, RuntimeNote *Note, uint32_t Lane) override;

        bool OnReleaseLane(double SongTime, RuntimeNote *Note, uint32_t Lane) override;

        TimingType GetTimingKind() override;
    };

    class O2JamMechanics : public Mechanics {
    public:

        bool OnUpdate(double SongBeat, RuntimeNote *Note, uint32_t Lane) override;

        bool OnPressLane(double SongBeat, RuntimeNote *Note, uint32_t Lane) override;

        bool OnReleaseLane(double SongBeat, RuntimeNote *Note, uint32_t Lane) override;

        TimingType GetTimingKind() override;
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
        EScratchState ScratchState[2];

        int GetScratchForLane(uint32_t Lane);

        bool CanHitNoteHead(double time, RuntimeNote *note);

        bool CanHitNoteTail(double time, RuntimeNote *note);

        void JudgeScratch(double SongTime, RuntimeNote *Note, uint32_t Lane, EScratchState newScratchState,
                          EScratchState oldScratchState);

        void PerformJudgement(double SongTime, RuntimeNote *Note, uint32_t Lane);

    public:
        RaindropArcadeMechanics();

        ~RaindropArcadeMechanics() = default;

        // If returns true, don't judge any more notes.
        bool OnUpdate(double SongTime, RuntimeNote *Note, uint32_t Lane) override;

        // If returns true, don't judge any more notes.
        bool OnPressLane(double SongTime, RuntimeNote *Note, uint32_t Lane) override;

        // If returns true, don't judge any more notes either.
        bool OnReleaseLane(double SongTime, RuntimeNote *Note, uint32_t Lane) override;

        bool OnScratchUp(double SongTime, RuntimeNote *Note, uint32_t Lane) override;

        bool OnScratchDown(double SongTime, RuntimeNote *Note, uint32_t Lane) override;

        // If Note is null, it didn't happen while an hold was being held
        // otherwise, it happened while a hold was being held
        bool OnScratchNeutral(double SongTime, RuntimeNote *Note, uint32_t Lane) override;

        TimingType GetTimingKind() override;
    };
}
