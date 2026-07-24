#include <cstdint>
#include <stdexcept>

#include <game/GameConstants.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper.h>

using namespace rd;

int RaindropArcadeMechanics::get_scratch_for_lane(const uint32_t lane) {
    if (lane == SCRATCH_1P_CHANNEL) {
        return 0;
    } else if (lane == SCRATCH_2P_CHANNEL) {
        return 1;
    } else {
        throw std::runtime_error("Scratch called on non-scratch lane!");
    }

    return 0;
}

bool RaindropArcadeMechanics::can_hit_note_head(const double time, RuntimeNote *note) {
    const double cutoff = player_score_keeper_->get_judgment_cutoff_ms() / 1000.0;
    return (abs(time - note->get_start_time()) < cutoff) && note->is_head_enabled() && !note->was_hit();
}

bool RaindropArcadeMechanics::can_hit_note_tail(double time, RuntimeNote *note) {
    return !note->is_head_enabled() && note->was_hit();
}

void RaindropArcadeMechanics::judge_scratch(const double song_time, RuntimeNote *note, const uint32_t lane,
                                           const EScratchState new_scratch_state, const EScratchState old_scratch_state) {

    if ((new_scratch_state != SCR_NEUTRAL && old_scratch_state == SCR_NEUTRAL) ||
        (old_scratch_state == SCR_UP && new_scratch_state == SCR_DOWN) ||
        (old_scratch_state == SCR_DOWN && new_scratch_state == SCR_UP)) {

        perform_judgement(song_time, note, lane);
    }
}

void RaindropArcadeMechanics::perform_judgement(const double song_time, RuntimeNote *note, const uint32_t lane) {
    // From neutral or opposite scratch, or key press, trigger the head.
    const double dev = 1000. * (song_time - note->get_start_time());
    if (is_early_miss(song_time, note)) {
        notify_miss(dev, lane, note->is_hold(), false, true);
    } else {
        // Heads, Non-holds
        if (note->is_head_enabled() && !note->was_hit()) {
            // Within miss judgement?
            if (!is_bm_bad_judge(song_time, note)) {
                // Hit head
                note->hit();
                notify_hit(dev, lane, note->is_hold() ? NoteJudgmentPart::HOLD_HEAD : NoteJudgmentPart::NOTE);

                play_keysound(note->get_sound());

                if (note->is_hold()) {
                    set_lane_holding_state(lane, true);
                    note->disable_head();
                } else {
                    note->disable();
                    note->make_invisible(); // Should we do this?
                }

            } else {
                // Completely disable head or note
                note->fail_hit();

                if (note->is_hold())
                    note->disable_head();
                else
                    note->disable();

                notify_miss(dev, lane, note->is_hold(), false, false);
            }
        } else { // Hold Tails

            note->disable();

            const double tdev = (note->get_end_time() - song_time) * 1000.;
            // Tail is within judge window, and head was hit
            if (abs(tdev) < player_score_keeper_->get_judgment_window(SKJ_W3)
                && note->was_hit()) {
                note->hit();
                notify_hit(tdev, lane, NoteJudgmentPart::HOLD_TAIL);
            } else { // Tail outside judgement
                note->fail_hit();
                notify_miss(dev, lane, note->is_hold(), false, false);
            }

            set_lane_holding_state(lane, false);
        }
    }
}

RaindropArcadeMechanics::RaindropArcadeMechanics() {
    scratch_state_[0] = scratch_state_[1] = SCR_NEUTRAL;
}

bool RaindropArcadeMechanics::on_update(const double song_time, RuntimeNote *note, const uint32_t lane) {
    if (!note->is_enabled()) return false;

    const double miss_time = player_score_keeper_->get_judgment_window(SKJ_W3);
    const double dev = (song_time - note->get_start_time()) * 1000.;
    const double tail_dev = (song_time - note->get_end_time()) * 1000.;

    if ((dev > miss_time && note->is_head_enabled()) ||  // Judge head only if not hit or regular note
        (tail_dev > miss_time && !note->is_head_enabled())) { // Judge tail regardless of whether it was hit or not

        note->failed_hit();

        // Check for nonhold or deactivated head
        if (!note->is_hold() || !note->is_head_enabled()) {
            note->disable();

            if (note->was_hit() && note->is_hold()) {
                set_lane_holding_state(lane, false);
            }
        }

        // Check hold with activated head
        if (note->is_hold() && note->is_head_enabled())
            note->disable_head();

        // Will "emergingly" miss head and tail at their respective times.

        notify_miss(dev, lane, note->is_hold(), false, false);
    }

    return false;
}

bool RaindropArcadeMechanics::on_press_lane(const double song_time, RuntimeNote *note, const uint32_t lane) {
    if (!note->is_enabled()) return false;
    if (!in_judge_cutoff(song_time, note)) return false;
    if (!can_hit_note_head(song_time, note)) return false;

    perform_judgement(song_time, note, lane);
    return true;
}

bool RaindropArcadeMechanics::on_release_lane(const double song_time, RuntimeNote *note, const uint32_t lane) {
    if (!note->is_enabled()) return false;
    if (!in_judge_cutoff(song_time, note)) return false;
    if (!can_hit_note_tail(song_time, note)) return false;

    perform_judgement(song_time, note, lane);
    return true;
}

bool RaindropArcadeMechanics::on_scratch_up(const double song_time, RuntimeNote *note, const uint32_t lane) {
    if (!note->is_enabled()) return false;
    const int scratch = get_scratch_for_lane(lane);
    const bool judge_head = can_hit_note_head(song_time, note);
    const bool judge_tail = can_hit_note_tail(song_time, note);
    if (!judge_head && !judge_tail) return false;

    judge_scratch(song_time, note, lane, SCR_UP, scratch_state_[scratch]);
    scratch_state_[scratch] = SCR_UP;
    return true;
}

bool RaindropArcadeMechanics::on_scratch_down(const double song_time, RuntimeNote *note, const uint32_t lane) {
    if (!note->is_enabled()) return false;
    const int scratch = get_scratch_for_lane(lane);
    const bool judge_head = can_hit_note_head(song_time, note);
    const bool judge_tail = can_hit_note_tail(song_time, note);
    if (!judge_head && !judge_tail) return false;

    judge_scratch(song_time, note, lane, SCR_DOWN, scratch_state_[scratch]);
    scratch_state_[scratch] = SCR_DOWN;
    return true;
}

bool RaindropArcadeMechanics::on_scratch_neutral(const double song_time, RuntimeNote *note, const uint32_t lane) {
    if (!note->is_enabled()) return false;
    const int scratch = get_scratch_for_lane(lane);
    const bool judge_head = can_hit_note_head(song_time, note);
    const bool judge_tail = can_hit_note_tail(song_time, note);
    if (!judge_head && !judge_tail) return false;

    if (scratch_state_[scratch] != SCR_NEUTRAL) {
        // It's an active hold? Then kill it.
        if (note->was_hit() && !note->is_head_enabled()) {
            note->fail_hit();
            note->disable();
        }
    } // else do nothing

    scratch_state_[scratch] = SCR_NEUTRAL;
    return true;
}

TimingType RaindropArcadeMechanics::get_timing_kind() {
    return TT_TIME;
}
