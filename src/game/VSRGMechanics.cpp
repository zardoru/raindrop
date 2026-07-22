#include <math.h>

#include "rmath.h"

#include <game/RaindropProcessedChart.h>

#include <game/ScoreKeeper.h>

#include <game/VSRGMechanics.h>
#include <game/NoteTransformations.h>


namespace rd {
    bool Mechanics::is_late_head_miss(double t, RuntimeNote *note) {
        return (t - note->get_start_time()) * 1000.0 > player_score_keeper_->get_late_miss_cutoff_ms();
    }

    bool Mechanics::in_judge_cutoff(double t, RuntimeNote *note) {
        double early_miss_cutoff = player_score_keeper_->get_early_miss_cutoff_ms() / 1000.0;
        double miss_cutoff = player_score_keeper_->get_late_miss_cutoff_ms() / 1000.0;
        return (abs(t - note->get_start_time()) <= early_miss_cutoff) ||
               (abs(t - note->get_end_time()) <= miss_cutoff);
    }

    bool Mechanics::is_early_miss(double t, RuntimeNote *note) {
        double dt = (t - note->get_start_time()) * 1000.;
        return dt < -player_score_keeper_->get_early_hit_cutoff_ms() && dt >= -player_score_keeper_->get_early_miss_cutoff_ms();
    }

    bool Mechanics::is_bm_bad_judge(double t, RuntimeNote *note) {
        double dt = abs(t - note->get_start_time()) * 1000.0;
        return dt > player_score_keeper_->get_judgment_window(SKJ_W3) && dt < player_score_keeper_->get_judgment_window(SKJ_W4);
    }

    bool Mechanics::in_head_cutoff(double t, RuntimeNote *note) {
        double dev = (t - note->get_start_time()) * 1000;
        return dev >= -player_score_keeper_->get_early_miss_cutoff_ms() &&
               dev <= player_score_keeper_->get_late_miss_cutoff_ms();
    }


    void Mechanics::transform_notes(RaindropProcessedChart &chart_state) {
        if (get_timing_kind() == TT_BEATS) {
            NoteTransform::TransformToBeats(
                    chart_state.chart->channels,
                    chart_state.notes,
                    chart_state.bps);
        }
    }

    void Mechanics::configure(otoworm::Chart *chart, std::shared_ptr<ScoreKeeper> score_keeper) {
        current_chart_ = chart;
        player_score_keeper_ = score_keeper;
    }

    RaindropMechanics::RaindropMechanics(bool forced_release) {
        this->forced_release_ = forced_release;
        hold_hit_time_.fill(NAN);
    }

    bool RaindropMechanics::on_update(double song_time, RuntimeNote *m, uint32_t lane) {
        auto k = lane;
        /* We have to check for all gameplay conditions for this note. */
        double miss_cutoff = player_score_keeper_->get_late_miss_cutoff_ms();

        // Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
        // note wasn't hit at the head and can't be hit at the head, and it's a hold
        if (!in_head_cutoff(song_time, m) // head outside judgment
            && !m->was_hit() && m->is_hold()) // not hit yet
        {
            double dev = (song_time - m->get_end_time()) * 1000;
            double t_d = abs(dev);

            if (dev > 0) {
                // remove hold notes that were never hit.
                m->make_invisible();

                if (notify_miss)
                    notify_miss(t_d, k, m->is_hold(), true, false);

                m->hit();

                return true;
            }

        } // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
        else if (is_late_head_miss(song_time, m) &&
                 (!m->was_hit() && m->is_head_enabled())) {
            if (notify_miss)
                notify_miss(
                        abs(song_time - m->get_start_time()) * 1000,
                        k,
                        m->is_hold(),
                        false,
                        false
                );

            // only remove tap notes from judgment; hold notes might be activated before the tail later.
            if (!(m->is_hold())) {
                m->make_invisible();
                m->disable();
            } else {
                m->disable_head();
                if (is_lane_key_down(k)) { // if the note was already being held down
                    m->hit();

                    if (set_lane_holding_state)
                        set_lane_holding_state(k, true);

                    hold_hit_time_[lane] = song_time;
                }
            }

            return true;
        } // Condition C: Hold head was hit, but hold tail was not released.
        else if (m->is_hold() && m->is_enabled() && m->was_hit()) {
            // Condition C-1: Forced release is enabled
            if ((song_time - m->get_end_time()) * 1000 > miss_cutoff && forced_release_) {
                m->fail_hit();
                // Take away health and combo (1st false)

                if (notify_miss)
                    notify_miss(abs(song_time - m->get_end_time()) * 1000, k, m->is_hold(), false, false);

                if (set_lane_holding_state)
                    set_lane_holding_state(k, false);

                m->disable();
                hold_hit_time_[lane] = NAN;

                return true;
            } else if ((song_time - m->get_end_time()) * 1000 > 0 && !forced_release_) {
                // Condition C-2: Forced release is not enabled
                if (is_lane_key_down(lane)) {
                    if (notify_hit)
                        notify_hit(0, k, true, true);
                } else {
                    // Only take away health, but not combo (1st true)
                    if (notify_miss)
                        notify_miss(
                                player_score_keeper_->get_late_miss_cutoff_ms(),
                                k,
                                m->is_hold(),
                                true,
                                false
                        );
                }

                if (set_lane_holding_state)
                    set_lane_holding_state(k, false);

                hold_hit_time_[lane] = NAN;
                m->disable();
                return true;
            } else { // still not over, and we're hitting it
                auto tick_interval = player_score_keeper_->get_ln_tick_interval();
                if (tick_interval > 0) {
                    if (m->is_enabled() && m->was_hit() && !::isnan(hold_hit_time_[lane])) {
                        auto delta = song_time - hold_hit_time_[lane];

                        if (delta > tick_interval) {
                            auto ticks = floor(delta / tick_interval);
                            player_score_keeper_->tick_ln((int) ticks);
                            hold_hit_time_[lane] += ticks * tick_interval;
                        }

                    }
                }
            }
        } // Condition D: Hold head was hit, but was released early was already handled at ReleaseLane so no need to be redundant here.

        return false;
    }

    bool RaindropMechanics::on_press_lane(double song_time, RuntimeNote *m, uint32_t lane) {
        if (!m->is_enabled())
            return false;

        double dev = (song_time - m->get_start_time()) * 1000;

        if (!in_head_cutoff(song_time, m)) // If the note was hit outside of judging range
        {
            // Log::Printf("td > jc %f %f\n", t_d, score_keeper->getJudgmentCutoff());
            // do nothing else for this note - anyway, this case happens if note optimization is disabled.
            return false;
        } else // Within judging range, including early misses
        {
            // early miss
            if (is_early_miss(song_time, m)) {
                if (notify_miss)
                    notify_miss(dev, lane, m->is_hold(), m->is_hold(), true);
            } else {
                m->hit();
                if (notify_hit)
                    notify_hit(dev, lane, m->is_hold(), false);

                if (m->is_hold()) {
                    if (set_lane_holding_state)
                        set_lane_holding_state(lane, true);

                    hold_hit_time_[lane] = song_time;
                } else {
                    m->disable();
                    m->make_invisible();
                }
            }

            if (play_keysound)
                play_keysound(m->get_sound());

            return true;
        }

        return false;
    }

    bool RaindropMechanics::on_release_lane(double song_time, RuntimeNote *m, uint32_t lane) {
        if (m->is_hold() && m->was_hit() &&
            m->is_enabled()) /* We hit the hold's head and we've not released it early already */
        {
            double dev = (song_time - m->get_end_time()) * 1000;
            double t_d = abs(dev);

            double early_hit = player_score_keeper_->get_early_miss_cutoff_ms();
            double late_miss = player_score_keeper_->get_late_miss_cutoff_ms();

            double release_window;

            if (forced_release_) {
                release_window = player_score_keeper_->get_judgment_window(SKJ_W3);
            } else
                release_window = 250; // 250 ms

            /* Released in time */
            if (((t_d < release_window) && !forced_release_) ||
                (dev > -early_hit && dev < late_miss)) {
                // Only consider it a timed thing if releasing it is forced.
                if (notify_hit)
                    notify_hit(forced_release_ ? dev : 0, lane, true, true);
            } else /* Released off time */
            {
                // early misses for hold notes always count as regular misses.
                // they don't break combo when we're not doing forced releases.
                m->fail_hit();

                if (notify_miss)
                    notify_miss(dev, lane, true, false, false);
            }

            hold_hit_time_[lane] = NAN;

            if (set_lane_holding_state)
                set_lane_holding_state(lane, false);

            m->disable();

            if (play_keysound)
                play_keysound(m->get_tail_sound());

            return true;
        }

        return false;
    }

    TimingType RaindropMechanics::get_timing_kind() {
        return TT_TIME;
    }

    TimingType O2JamMechanics::get_timing_kind() {
        return TT_BEATS;
    }

    bool O2JamMechanics::on_release_lane(double song_beat, RuntimeNote *m, uint32_t lane) {
        if (m->is_hold() && m->was_hit() &&
            m->is_enabled()) /* We hit the hold's head and we've not released it early already */
        {
            double dev = (song_beat - m->get_end_time());
            double t_d = abs(dev);

            if (t_d < player_score_keeper_->get_judgment_window(SKJ_W3)) /* Released in time */
            {
                notify_hit(dev, lane, m->is_hold(), true);
                set_lane_holding_state(lane, false);
                m->disable();
            } else /* Released off time (early since Late is managed by the on_update function.) */
            {
                m->fail_hit();
                notify_miss(dev, lane, m->is_hold(), false, false);

                m->disable();
                set_lane_holding_state(lane, false);
            }

            play_keysound(m->get_tail_sound());

            return true;
        }

        return false;
    }

    bool O2JamMechanics::on_press_lane(double song_beat, RuntimeNote *m, uint32_t lane) {
        if (!m->is_enabled())
            return false;

        double dev = (song_beat - m->get_start_time());
        double t_d = abs(dev);

        if (t_d < player_score_keeper_->get_judgment_window(SKJ_W3)) // If the note was hit inside judging range
        {
            m->hit();

            notify_hit(dev, lane, m->is_hold(), false);

            if (m->is_hold())
                set_lane_holding_state(lane, true);
            else {
                m->disable();

                // BADs stay visible.
                if (t_d < player_score_keeper_->get_judgment_window(SKJ_W2))
                    m->make_invisible();
            }

            play_keysound(m->get_sound());

            return true;
        } else if (t_d > player_score_keeper_->get_judgment_window(SKJ_W3) && t_d < player_score_keeper_->get_late_miss_cutoff_ms()) {
            m->fail_hit();
            m->disable();

            notify_miss(dev, lane, m->is_hold(), false, false);
            play_keysound(m->get_sound());
        }

        return false;
    }

    bool O2JamMechanics::on_update(double song_beat, RuntimeNote *m, uint32_t lane) {
        auto k = lane;
        double t_tail = song_beat - m->get_end_time();
        double t_head = song_beat - m->get_start_time();

        if (!m->is_enabled()) return false; // keep looking

        // Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
        // note wasn't hit at the head and it's a hold
        if (t_tail > 0 && !m->was_hit() && m->is_hold()) {
            // remove hold notes that were never hit.
            m->fail_hit();
            notify_miss(abs(t_tail), k, m->is_hold(), true, false);
            m->disable();
        } // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
        else if (t_head > player_score_keeper_->get_judgment_window(SKJ_W3) && !m->was_hit() && m->is_enabled()) {
            m->fail_hit();
            notify_miss(abs(t_head), k, m->is_hold(), false, false);

            // remove from judgment completely
            m->disable();
        } // Condition C: Hold head was hit, but hold tail was not released.
        else if (t_tail > player_score_keeper_->get_judgment_window(SKJ_W3) &&
                 m->is_hold() && m->was_hit() && m->is_enabled()) {
            m->fail_hit();
            notify_miss(abs(t_tail), k, m->is_hold(), false, false);

            set_lane_holding_state(k, false);
            m->disable();
        }

        return false;
    }

    bool Mechanics::on_scratch_up(double song_time, RuntimeNote *note, uint32_t lane) {
        return false;
    }

    bool Mechanics::on_scratch_down(double song_time, RuntimeNote *note, uint32_t lane) {
        return false;
    }

    bool Mechanics::on_scratch_neutral(double song_time, RuntimeNote *note, uint32_t lane) {
        return false;
    }
}
