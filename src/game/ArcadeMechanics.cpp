#include <cstdint>
#include <stdexcept>

#include <game/GameConstants.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper.h>

using namespace rd;

int RaindropArcadeMechanics::GetScratchForLane(uint32_t Lane) {
    if (Lane == SCRATCH_1P_CHANNEL) {
        return 0;
    } else if (Lane == SCRATCH_2P_CHANNEL) {
        return 1;
    } else {
        throw std::runtime_error("Scratch called on non-scratch lane!");
    }

    return 0;
}

bool RaindropArcadeMechanics::CanHitNoteHead(double time, RuntimeNote *note) {
    double cutoff = PlayerScoreKeeper->getJudgmentCutoffMS() / 1000.0;
    return (abs(time - note->get_start_time()) < cutoff) && note->is_head_enabled() && !note->was_hit();
}

bool RaindropArcadeMechanics::CanHitNoteTail(double time, RuntimeNote *note) {
    return !note->is_head_enabled() && note->was_hit();
}

void RaindropArcadeMechanics::JudgeScratch(double SongTime, RuntimeNote *Note, uint32_t Lane,
                                           EScratchState newScratchState, EScratchState oldScratchState) {

    if ((newScratchState != SCR_NEUTRAL && oldScratchState == SCR_NEUTRAL) ||
        (oldScratchState == SCR_UP && newScratchState == SCR_DOWN) ||
        (oldScratchState == SCR_DOWN && newScratchState == SCR_UP)) {

        PerformJudgement(SongTime, Note, Lane);
    }
}

void RaindropArcadeMechanics::PerformJudgement(double SongTime, RuntimeNote *Note, uint32_t Lane) {
    // From neutral or opposite scratch, or key press, trigger the head.
    double dev = 1000. * (SongTime - Note->get_start_time());
    if (IsEarlyMiss(SongTime, Note)) {
        notify_miss(dev, Lane, Note->is_hold(), false, true);
    } else {
        // Heads, Non-holds
        if (Note->is_head_enabled() && !Note->was_hit()) {
            // Within miss judgement?
            if (!IsBmBadJudge(SongTime, Note)) {
                // Hit head
                Note->hit();
                notify_hit(dev, Lane, Note->is_hold(), false);

                play_keysound(Note->get_sound());

                if (Note->is_hold()) {
                    set_lane_holding_state(Lane, true);
                    Note->disable_head();
                } else {
                    Note->disable();
                    Note->make_invisible(); // Should we do this?
                }

            } else {
                // Completely disable head or note
                Note->fail_hit();

                if (Note->is_hold())
                    Note->disable_head();
                else
                    Note->disable();

                notify_miss(dev, Lane, Note->is_hold(), false, false);
            }
        } else { // Hold Tails

            Note->disable();

            double tdev = (Note->get_end_time() - SongTime) * 1000.;
            // Tail is within judge window, and head was hit
            if (abs(tdev) < PlayerScoreKeeper->getJudgmentWindow(SKJ_W3)
                && Note->was_hit()) {
                Note->hit();
                notify_hit(tdev, Lane, Note->is_hold(), true);
            } else { // Tail outside judgement
                Note->fail_hit();
                notify_miss(dev, Lane, Note->is_hold(), false, false);
            }

            set_lane_holding_state(Lane, false);
        }
    }
}

RaindropArcadeMechanics::RaindropArcadeMechanics() {
    ScratchState[0] = ScratchState[1] = SCR_NEUTRAL;
}

bool RaindropArcadeMechanics::OnUpdate(double SongTime, RuntimeNote *Note, uint32_t Lane) {
    if (!Note->is_enabled()) return false;

    double miss_time = PlayerScoreKeeper->getJudgmentWindow(SKJ_W3);
    double dev = (SongTime - Note->get_start_time()) * 1000.;
    double tail_dev = (SongTime - Note->get_end_time()) * 1000.;

    if ((dev > miss_time && Note->is_head_enabled()) ||  // Judge head only if not hit or regular note
        (tail_dev > miss_time && !Note->is_head_enabled())) { // Judge tail regardless of whether it was hit or not

        Note->failed_hit();

        // Check for nonhold or deactivated head
        if (!Note->is_hold() || !Note->is_head_enabled()) {
            Note->disable();

            if (Note->was_hit() && Note->is_hold()) {
                set_lane_holding_state(Lane, false);
            }
        }

        // Check hold with activated head
        if (Note->is_hold() && Note->is_head_enabled())
            Note->disable_head();

        // Will "emergingly" miss head and tail at their respective times.

        notify_miss(dev, Lane, Note->is_hold(), false, false);
    }

    return false;
}

bool RaindropArcadeMechanics::OnPressLane(double SongTime, RuntimeNote *Note, uint32_t Lane) {
    if (!Note->is_enabled()) return false;
    if (!InJudgeCutoff(SongTime, Note)) return false;
    if (!CanHitNoteHead(SongTime, Note)) return false;

    PerformJudgement(SongTime, Note, Lane);
    return true;
}

bool RaindropArcadeMechanics::OnReleaseLane(double SongTime, RuntimeNote *Note, uint32_t Lane) {
    if (!Note->is_enabled()) return false;
    if (!InJudgeCutoff(SongTime, Note)) return false;
    if (!CanHitNoteTail(SongTime, Note)) return false;

    PerformJudgement(SongTime, Note, Lane);
    return true;
}

bool RaindropArcadeMechanics::OnScratchUp(double SongTime, RuntimeNote *Note, uint32_t Lane) {
    if (!Note->is_enabled()) return false;
    int scratch = GetScratchForLane(Lane);
    bool judgeHead = CanHitNoteHead(SongTime, Note);
    bool judgeTail = CanHitNoteTail(SongTime, Note);
    if (!judgeHead && !judgeTail) return false;

    JudgeScratch(SongTime, Note, Lane, SCR_UP, ScratchState[scratch]);
    ScratchState[scratch] = SCR_UP;
    return true;
}

bool RaindropArcadeMechanics::OnScratchDown(double SongTime, RuntimeNote *Note, uint32_t Lane) {
    if (!Note->is_enabled()) return false;
    int scratch = GetScratchForLane(Lane);
    bool judgeHead = CanHitNoteHead(SongTime, Note);
    bool judgeTail = CanHitNoteTail(SongTime, Note);
    if (!judgeHead && !judgeTail) return false;

    JudgeScratch(SongTime, Note, Lane, SCR_DOWN, ScratchState[scratch]);
    ScratchState[scratch] = SCR_DOWN;
    return true;
}

bool RaindropArcadeMechanics::OnScratchNeutral(double SongTime, RuntimeNote *Note, uint32_t Lane) {
    if (!Note->is_enabled()) return false;
    int scratch = GetScratchForLane(Lane);
    bool judgeHead = CanHitNoteHead(SongTime, Note);
    bool judgeTail = CanHitNoteTail(SongTime, Note);
    if (!judgeHead && !judgeTail) return false;

    if (ScratchState[scratch] != SCR_NEUTRAL) {
        // It's an active hold? Then kill it.
        if (Note->was_hit() && !Note->is_head_enabled()) {
            Note->fail_hit();
            Note->disable();
        }
    } // else do nothing

    ScratchState[scratch] = SCR_NEUTRAL;
    return true;
}

TimingType RaindropArcadeMechanics::GetTimingKind() {
    return TT_TIME;
}
