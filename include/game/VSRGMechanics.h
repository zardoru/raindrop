#pragma once

#include <functional>
#include <game/PlayerChartState.h>

namespace rd {
    class TrackNote;
    class ScoreKeeper;
    class Song;

    class Mechanics {
    public:
        typedef std::function<void(double, uint32_t, bool, bool)> HitEvent;
        typedef std::function<void(double, uint32_t, bool, bool, bool)> MissEvent;
        typedef std::function<void(uint32_t)> KeysoundEvent;

    protected:

        Difficulty *CurrentDifficulty;
        std::shared_ptr<ScoreKeeper> PlayerScoreKeeper;
    public:

        bool IsLateHeadMiss(double t, TrackNote *note);

        bool InJudgeCutoff(double t, TrackNote *note);

        bool IsEarlyMiss(double t, TrackNote *note);

        bool IsBmBadJudge(double t, TrackNote *note);

        bool InHeadCutoff(double t, TrackNote *note);

        virtual ~Mechanics() = default;

        // These HAVE to be set before anything else is called.
        std::function<bool(uint32_t)> IsLaneKeyDown;
        std::function<void(uint32_t, bool)> SetLaneHoldingState;
        KeysoundEvent PlayNoteSoundEvent;
        HitEvent HitNotify;
        MissEvent MissNotify;

        virtual void TransformNotes(PlayerChartState &ChartState);

        virtual void Setup(Difficulty *Difficulty, std::shared_ptr<ScoreKeeper> scoreKeeper);

        // If returns true, don't judge any more notes.
        virtual bool OnUpdate(double SongTime, TrackNote *Note, uint32_t Lane) = 0;

        // If returns true, don't judge any more notes.
        virtual bool OnPressLane(double SongTime, TrackNote *Note, uint32_t Lane) = 0;

        // If returns true, don't judge any more notes either.
        virtual bool OnReleaseLane(double SongTime, TrackNote *Note, uint32_t Lane) = 0;

        virtual bool OnScratchUp(double SongTime, TrackNote *Note, uint32_t Lane);

        virtual bool OnScratchDown(double SongTime, TrackNote *Note, uint32_t Lane);

        virtual bool OnScratchNeutral(double SongTime, TrackNote *Note, uint32_t Lane);

        virtual TimingType GetTimingKind() = 0;
    };

    class RaindropMechanics : public Mechanics {
        bool forcedRelease;
    public:
        RaindropMechanics(bool forcedRelease);

        bool OnUpdate(double SongTime, TrackNote *Note, uint32_t Lane) override;

        bool OnPressLane(double SongTime, TrackNote *Note, uint32_t Lane) override;

        bool OnReleaseLane(double SongTime, TrackNote *Note, uint32_t Lane) override;

        TimingType GetTimingKind() override;
    };

    class O2JamMechanics : public Mechanics {
    public:

        bool OnUpdate(double SongBeat, TrackNote *Note, uint32_t Lane) override;

        bool OnPressLane(double SongBeat, TrackNote *Note, uint32_t Lane) override;

        bool OnReleaseLane(double SongBeat, TrackNote *Note, uint32_t Lane) override;

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

        bool CanHitNoteHead(double time, TrackNote *note);

        bool CanHitNoteTail(double time, TrackNote *note);

        void JudgeScratch(double SongTime, TrackNote *Note, uint32_t Lane, EScratchState newScratchState,
                          EScratchState oldScratchState);

        void PerformJudgement(double SongTime, TrackNote *Note, uint32_t Lane);

    public:
        RaindropArcadeMechanics();

        ~RaindropArcadeMechanics() = default;

        // If returns true, don't judge any more notes.
        bool OnUpdate(double SongTime, TrackNote *Note, uint32_t Lane) override;

        // If returns true, don't judge any more notes.
        bool OnPressLane(double SongTime, TrackNote *Note, uint32_t Lane) override;

        // If returns true, don't judge any more notes either.
        bool OnReleaseLane(double SongTime, TrackNote *Note, uint32_t Lane) override;

        bool OnScratchUp(double SongTime, TrackNote *Note, uint32_t Lane) override;

        bool OnScratchDown(double SongTime, TrackNote *Note, uint32_t Lane) override;

        // If Note is null, it didn't happen while an hold was being held
        // otherwise, it happened while a hold was being held
        bool OnScratchNeutral(double SongTime, TrackNote *Note, uint32_t Lane) override;

        TimingType GetTimingKind() override;
    };
}