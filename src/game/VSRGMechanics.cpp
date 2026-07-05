#include <math.h>

#include "rmath.h"

#include <game/Song.h>
#include <game/RaindropProcessedChart.h>

#include <game/ScoreKeeper.h>

#include <game/VSRGMechanics.h>
#include <game/NoteTransformations.h>


namespace rd {
    bool Mechanics::IsLateHeadMiss(double t, RuntimeNote *note) {
        return (t - note->get_start_time()) * 1000.0 > PlayerScoreKeeper->getLateMissCutoffMS();
    }

    bool Mechanics::InJudgeCutoff(double t, RuntimeNote *note) {
        double earlyMissCutoff = PlayerScoreKeeper->getEarlyMissCutoffMS() / 1000.0;
        double missCutoff = PlayerScoreKeeper->getLateMissCutoffMS() / 1000.0;
        return (abs(t - note->get_start_time()) <= earlyMissCutoff) ||
               (abs(t - note->get_end_time()) <= missCutoff);
    }

    bool Mechanics::IsEarlyMiss(double t, RuntimeNote *note) {
        double dt = (t - note->get_start_time()) * 1000.;
        return dt < -PlayerScoreKeeper->getEarlyHitCutoffMS() && dt >= -PlayerScoreKeeper->getEarlyMissCutoffMS();
    }

    bool Mechanics::IsBmBadJudge(double t, RuntimeNote *note) {
        double dt = abs(t - note->get_start_time()) * 1000.0;
        return dt > PlayerScoreKeeper->getJudgmentWindow(SKJ_W3) && dt < PlayerScoreKeeper->getJudgmentWindow(SKJ_W4);
    }

    bool Mechanics::InHeadCutoff(double t, RuntimeNote *note) {
        double dev = (t - note->get_start_time()) * 1000;
        return dev >= -PlayerScoreKeeper->getEarlyMissCutoffMS() &&
               dev <= PlayerScoreKeeper->getLateMissCutoffMS();
    }


    void Mechanics::transform_notes(RaindropProcessedChart &ChartState) {
        if (GetTimingKind() == TT_BEATS) {
            NoteTransform::TransformToBeats(
                    ChartState.chart->channels,
                    ChartState.notes,
                    ChartState.bps);
        }
    }

    void Mechanics::configure(otoworm::Chart *chart, std::shared_ptr<ScoreKeeper> scoreKeeper) {
        CurrentChart = chart;
        PlayerScoreKeeper = scoreKeeper;
    }

    RaindropMechanics::RaindropMechanics(bool forcedRelease) {
        this->forcedRelease = forcedRelease;
        hold_hit_time.fill(NAN);
    }

    bool RaindropMechanics::OnUpdate(double SongTime, RuntimeNote *m, uint32_t Lane) {
        auto k = Lane;
        /* We have to check for all gameplay conditions for this note. */
        double missCutoff = PlayerScoreKeeper->getLateMissCutoffMS();

        // Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
        // note wasn't hit at the head and can't be hit at the head, and it's a hold
        if (!InHeadCutoff(SongTime, m) // head outside judgment
            && !m->was_hit() && m->is_hold()) // not hit yet
        {
            double dev = (SongTime - m->get_end_time()) * 1000;
            double tD = abs(dev);

            if (dev > 0) {
                // remove hold notes that were never hit.
                m->make_invisible();

                if (notify_miss)
                    notify_miss(tD, k, m->is_hold(), true, false);

                m->hit();

                return true;
            }

        } // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
        else if (IsLateHeadMiss(SongTime, m) &&
                 (!m->was_hit() && m->is_head_enabled())) {
            if (notify_miss)
                notify_miss(
                        abs(SongTime - m->get_start_time()) * 1000,
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

                    hold_hit_time[Lane] = SongTime;
                }
            }

            return true;
        } // Condition C: Hold head was hit, but hold tail was not released.
        else if (m->is_hold() && m->is_enabled() && m->was_hit()) {
            // Condition C-1: Forced release is enabled
            if ((SongTime - m->get_end_time()) * 1000 > missCutoff && forcedRelease) {
                m->fail_hit();
                // Take away health and combo (1st false)

                if (notify_miss)
                    notify_miss(abs(SongTime - m->get_end_time()) * 1000, k, m->is_hold(), false, false);

                if (set_lane_holding_state)
                    set_lane_holding_state(k, false);

                m->disable();
                hold_hit_time[Lane] = NAN;

                return true;
            } else if ((SongTime - m->get_end_time()) * 1000 > 0 && !forcedRelease) {
                // Condition C-2: Forced release is not enabled
                if (is_lane_key_down(Lane)) {
                    if (notify_hit)
                        notify_hit(0, k, true, true);
                } else {
                    // Only take away health, but not combo (1st true)
                    if (notify_miss)
                        notify_miss(
                                PlayerScoreKeeper->getLateMissCutoffMS(),
                                k,
                                m->is_hold(),
                                true,
                                false
                        );
                }

                if (set_lane_holding_state)
                    set_lane_holding_state(k, false);

                hold_hit_time[Lane] = NAN;
                m->disable();
                return true;
            } else { // still not over, and we're hitting it
                auto tick_interval = PlayerScoreKeeper->getLNTickInterval();
                if (tick_interval > 0) {
                    if (m->is_enabled() && m->was_hit() && !::isnan(hold_hit_time[Lane])) {
                        auto delta = SongTime - hold_hit_time[Lane];

                        if (delta > tick_interval) {
                            auto ticks = floor(delta / tick_interval);
                            PlayerScoreKeeper->tickLN((int) ticks);
                            hold_hit_time[Lane] += ticks * tick_interval;
                        }

                    }
                }
            }
        } // Condition D: Hold head was hit, but was released early was already handled at ReleaseLane so no need to be redundant here.

        return false;
    }

    bool RaindropMechanics::OnPressLane(double SongTime, RuntimeNote *m, uint32_t Lane) {
        if (!m->is_enabled())
            return false;

        double dev = (SongTime - m->get_start_time()) * 1000;

        if (!InHeadCutoff(SongTime, m)) // If the note was hit outside of judging range
        {
            // Log::Printf("td > jc %f %f\n", tD, score_keeper->getJudgmentCutoff());
            // do nothing else for this note - anyway, this case happens if note optimization is disabled.
            return false;
        } else // Within judging range, including early misses
        {
            // early miss
            if (IsEarlyMiss(SongTime, m)) {
                if (notify_miss)
                    notify_miss(dev, Lane, m->is_hold(), m->is_hold(), true);
            } else {
                m->hit();
                if (notify_hit)
                    notify_hit(dev, Lane, m->is_hold(), false);

                if (m->is_hold()) {
                    if (set_lane_holding_state)
                        set_lane_holding_state(Lane, true);

                    hold_hit_time[Lane] = SongTime;
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

    bool RaindropMechanics::OnReleaseLane(double SongTime, RuntimeNote *m, uint32_t Lane) {
        if (m->is_hold() && m->was_hit() &&
            m->is_enabled()) /* We hit the hold's head and we've not released it early already */
        {
            double dev = (SongTime - m->get_end_time()) * 1000;
            double tD = abs(dev);

            double earlyHit = PlayerScoreKeeper->getEarlyMissCutoffMS();
            double lateMiss = PlayerScoreKeeper->getLateMissCutoffMS();

            double releaseWindow;

            if (forcedRelease) {
                releaseWindow = PlayerScoreKeeper->getJudgmentWindow(SKJ_W3);
            } else
                releaseWindow = 250; // 250 ms

            /* Released in time */
            if (((tD < releaseWindow) && !forcedRelease) ||
                (dev > -earlyHit && dev < lateMiss)) {
                // Only consider it a timed thing if releasing it is forced.
                if (notify_hit)
                    notify_hit(forcedRelease ? dev : 0, Lane, true, true);
            } else /* Released off time */
            {
                // early misses for hold notes always count as regular misses.
                // they don't break combo when we're not doing forced releases.
                m->fail_hit();

                if (notify_miss)
                    notify_miss(dev, Lane, true, false, false);
            }

            hold_hit_time[Lane] = NAN;

            if (set_lane_holding_state)
                set_lane_holding_state(Lane, false);

            m->disable();

            if (play_keysound)
                play_keysound(m->get_tail_sound());

            return true;
        }

        return false;
    }

    TimingType RaindropMechanics::GetTimingKind() {
        return TT_TIME;
    }

    TimingType O2JamMechanics::GetTimingKind() {
        return TT_BEATS;
    }

    bool O2JamMechanics::OnReleaseLane(double SongBeat, RuntimeNote *m, uint32_t Lane) {
        if (m->is_hold() && m->was_hit() &&
            m->is_enabled()) /* We hit the hold's head and we've not released it early already */
        {
            double dev = (SongBeat - m->get_end_time());
            double tD = abs(dev);

            if (tD < PlayerScoreKeeper->getJudgmentWindow(SKJ_W3)) /* Released in time */
            {
                notify_hit(dev, Lane, m->is_hold(), true);
                set_lane_holding_state(Lane, false);
                m->disable();
            } else /* Released off time (early since Late is managed by the OnUpdate function.) */
            {
                m->fail_hit();
                notify_miss(dev, Lane, m->is_hold(), false, false);

                m->disable();
                set_lane_holding_state(Lane, false);
            }

            play_keysound(m->get_tail_sound());

            return true;
        }

        return false;
    }

    bool O2JamMechanics::OnPressLane(double SongBeat, RuntimeNote *m, uint32_t Lane) {
        if (!m->is_enabled())
            return false;

        double dev = (SongBeat - m->get_start_time());
        double tD = abs(dev);

        if (tD < PlayerScoreKeeper->getJudgmentWindow(SKJ_W3)) // If the note was hit inside judging range
        {
            m->hit();

            notify_hit(dev, Lane, m->is_hold(), false);

            if (m->is_hold())
                set_lane_holding_state(Lane, true);
            else {
                m->disable();

                // BADs stay visible.
                if (tD < PlayerScoreKeeper->getJudgmentWindow(SKJ_W2))
                    m->make_invisible();
            }

            play_keysound(m->get_sound());

            return true;
        } else if (tD > PlayerScoreKeeper->getJudgmentWindow(SKJ_W3) && tD < PlayerScoreKeeper->getLateMissCutoffMS()) {
            m->fail_hit();
            m->disable();

            notify_miss(dev, Lane, m->is_hold(), false, false);
            play_keysound(m->get_sound());
        }

        return false;
    }

    bool O2JamMechanics::OnUpdate(double SongBeat, RuntimeNote *m, uint32_t Lane) {
        auto k = Lane;
        double tTail = SongBeat - m->get_end_time();
        double tHead = SongBeat - m->get_start_time();

        if (!m->is_enabled()) return false; // keep looking

        // Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
        // note wasn't hit at the head and it's a hold
        if (tTail > 0 && !m->was_hit() && m->is_hold()) {
            // remove hold notes that were never hit.
            m->fail_hit();
            notify_miss(abs(tTail), k, m->is_hold(), true, false);
            m->disable();
        } // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
        else if (tHead > PlayerScoreKeeper->getJudgmentWindow(SKJ_W3) && !m->was_hit() && m->is_enabled()) {
            m->fail_hit();
            notify_miss(abs(tHead), k, m->is_hold(), false, false);

            // remove from judgment completely
            m->disable();
        } // Condition C: Hold head was hit, but hold tail was not released.
        else if (tTail > PlayerScoreKeeper->getJudgmentWindow(SKJ_W3) &&
                 m->is_hold() && m->was_hit() && m->is_enabled()) {
            m->fail_hit();
            notify_miss(abs(tTail), k, m->is_hold(), false, false);

            set_lane_holding_state(k, false);
            m->disable();
        }

        return false;
    }

    bool Mechanics::OnScratchUp(double SongTime, RuntimeNote *Note, uint32_t Lane) {
        return false;
    }

    bool Mechanics::OnScratchDown(double SongTime, RuntimeNote *Note, uint32_t Lane) {
        return false;
    }

    bool Mechanics::OnScratchNeutral(double SongTime, RuntimeNote *Note, uint32_t Lane) {
        return false;
    }
}
