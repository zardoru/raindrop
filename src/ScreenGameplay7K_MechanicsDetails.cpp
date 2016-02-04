#include "pch.h"

#include "GameGlobal.h"
#include "Screen.h"
#include "Sprite.h"
#include "ImageList.h"

#include "ScreenGameplay7K.h"
#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K_Mechanics.h"

void VSRGMechanics::Setup(VSRG::Song *Song, VSRG::Difficulty *Difficulty, std::shared_ptr<ScoreKeeper7K> scoreKeeper)
{
    CurrentSong = Song;
    CurrentDifficulty = Difficulty;
    score_keeper = scoreKeeper;
}

RaindropMechanics::RaindropMechanics(bool forcedRelease)
{
    this->forcedRelease = forcedRelease;
}

bool RaindropMechanics::OnUpdate(double SongTime, VSRG::TrackNote* m, uint32_t Lane)
{
    auto k = Lane;
    /* We have to check for all gameplay conditions for this note. */

    // Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
    // note wasn't hit at the head and it's a hold
    if ((SongTime - m->GetTimeFinal()) > 0 && !m->WasNoteHit() && m->IsHold())
    {
        // ^ no need for delays here.
        // remove hold notes that were never hit.
        m->MakeInvisible();
        MissNotify(abs(SongTime - m->GetTimeFinal()) * 1000, k, m->IsHold(), true, false);
        m->Hit();
    } // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
    else if ((SongTime - m->GetStartTime()) * 1000 > score_keeper->getMissCutoff() &&
        (!m->WasNoteHit() && m->IsHeadEnabled()))
    {
        MissNotify(abs(SongTime - m->GetStartTime()) * 1000, k, m->IsHold(), false, false);

        // only remove tap notes from judgment; hold notes might be activated before the tail later.
        if (!(m->IsHold()))
        {
            m->MakeInvisible();
            m->Disable();
        }
        else
        {
            m->DisableHead();
            if (IsLaneKeyDown(k))
            { // if the note was already being held down
                m->Hit();
                SetLaneHoldingState(k, true);
            }
        }
    } // Condition C: Hold head was hit, but hold tail was not released.
    else if (m->IsHold() && m->IsEnabled())
    {
        // Condition C-1: Forced release is enabled
        if ((SongTime - m->GetTimeFinal()) * 1000 > score_keeper->getMissCutoff() && forcedRelease)
        {
            m->FailHit();
            MissNotify(abs(SongTime - m->GetTimeFinal()) * 1000, k, m->IsHold(), true, false);

            SetLaneHoldingState(k, false);
            m->Disable();
        }
        else
        {
            // Condition C-2: Forced release is not enabled
            if (SongTime - m->GetTimeFinal() > 0 && !forcedRelease)
            {
                if (IsLaneKeyDown(Lane))
                {
                    HitNotify(0, k, true, true);
                }
                else
                {
                    MissNotify(score_keeper->getMissCutoff(), k, m->IsHold(), true, false);
                }

                SetLaneHoldingState(k, false);
                m->Disable();
            }
        }
    } // Condition D: Hold head was hit, but was released early was already handled at ReleaseLane so no need to be redundant here.

    return false;
}

bool RaindropMechanics::OnPressLane(double SongTime, VSRG::TrackNote* m, uint32_t Lane)
{
    if (!m->IsEnabled())
        return false;

    double dev = (SongTime - m->GetStartTime()) * 1000;
    double tD = abs(dev);

    if (tD > score_keeper->getEarlyMissCutoff()) // If the note was hit outside of judging range
    {
        // do nothing else for this note
        return false;
    }
    else // Within judging range, including early misses
    {
        // early miss
        if (dev < -(score_keeper->getMissCutoff()))
        {
            MissNotify(dev, Lane, m->IsHold(), m->IsHold(), true);
        }
        else
        {
            m->Hit();
            HitNotify(dev, Lane, m->IsHold(), false);

            if (m->IsHold())
                SetLaneHoldingState(Lane, true);
            else
            {
                m->Disable();
                m->MakeInvisible();
            }
        }

        PlayNoteSoundEvent(m);
        return true;
    }

    return false;
}

bool RaindropMechanics::OnReleaseLane(double SongTime, VSRG::TrackNote* m, uint32_t Lane)
{
    if (m->IsHold() && m->WasNoteHit() && m->IsEnabled()) /* We hit the hold's head and we've not released it early already */
    {
        double dev = (SongTime - m->GetTimeFinal()) * 1000;
        double tD = abs(dev);

        double releaseWindow;

        if (forcedRelease)
            releaseWindow = score_keeper->getJudgmentWindow(SKJ_W3);
        else
            releaseWindow = 250; // 250 ms

        if (tD < releaseWindow) /* Released in time */
        {
            // Only consider it a timed thing if releasing it is forced.
            HitNotify(forcedRelease ? dev : 0, Lane, true, true);
            SetLaneHoldingState(Lane, false);
            m->Disable();
        }
        else /* Released off time */
        {
            // early misses for hold notes always count as regular misses.
            // they don't break combo when we're not doing forced releases.
            m->FailHit();
            MissNotify(dev, Lane, true, false, false);

            m->Disable();
            SetLaneHoldingState(Lane, false);
        }
        return true;
    }

    return false;
}

TimingType RaindropMechanics::GetTimingKind()
{
    return TT_TIME;
}

TimingType O2JamMechanics::GetTimingKind()
{
    return TT_BEATS;
}

bool O2JamMechanics::OnReleaseLane(double SongBeat, VSRG::TrackNote* m, uint32_t Lane)
{
    if (m->IsHold() && m->WasNoteHit() && m->IsEnabled()) /* We hit the hold's head and we've not released it early already */
    {
        double dev = (SongBeat - m->GetTimeFinal());
        double tD = abs(dev);

        if (tD < score_keeper->getJudgmentWindow(SKJ_W3)) /* Released in time */
        {
            HitNotify(dev, Lane, m->IsHold(), true);
            SetLaneHoldingState(Lane, false);
            m->Disable();
        }
        else /* Released off time (early since Late is managed by the OnUpdate function.) */
        {
            m->FailHit();
            MissNotify(dev, Lane, m->IsHold(), false, false);

            m->Disable();
            SetLaneHoldingState(Lane, false);
        }
        return true;
    }

    return false;
}

bool O2JamMechanics::OnPressLane(double SongBeat, VSRG::TrackNote* m, uint32_t Lane)
{
    if (!m->IsEnabled())
        return false;

    double dev = (SongBeat - m->GetStartTime());
    double tD = abs(dev);

    if (tD < score_keeper->getJudgmentWindow(SKJ_W3)) // If the note was hit inside judging range
    {
        m->Hit();

        HitNotify(dev, Lane, m->IsHold(), false);

        if (m->IsHold())
            SetLaneHoldingState(Lane, true);
        else
        {
            m->Disable();

            // BADs stay visible.
            if (tD < score_keeper->getJudgmentWindow(SKJ_W2))
                m->MakeInvisible();
        }

        PlayNoteSoundEvent(m);

        return true;
    }
    return false;
}

bool O2JamMechanics::OnUpdate(double SongBeat, VSRG::TrackNote* m, uint32_t Lane)
{
    auto k = Lane;
    double tD = SongBeat - m->GetTimeFinal();
    double tHead = SongBeat - m->GetStartTime();

    // Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
    // note wasn't hit at the head and it's a hold
    if (tD > 0 && !m->WasNoteHit() && m->IsHold())
    {
        // remove hold notes that were never hit.
        m->FailHit();
        MissNotify(abs(tD), k, m->IsHold(), true, false);
        m->Hit();
        m->Disable();
    } // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
    else if (tHead > score_keeper->getMissCutoff() && !m->WasNoteHit() && m->IsEnabled())
    {
        m->FailHit();
        MissNotify(abs(tD), k, m->IsHold(), false, false);

        // remove from judgment completely
        m->Disable();
    } // Condition C: Hold head was hit, but hold tail was not released.
    else if (tD > score_keeper->getMissCutoff() &&
        m->IsHold() && m->WasNoteHit() && m->IsEnabled())
    {
        m->FailHit();
        MissNotify(abs(tD), k, m->IsHold(), false, false);

        SetLaneHoldingState(k, false);
        m->Disable();
    }

    return false;
}