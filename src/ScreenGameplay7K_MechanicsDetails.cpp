#include "GameGlobal.h"
#include "Screen.h"
#include "GraphObject2D.h"
#include "ImageList.h"

#include <map>
#include "ScreenGameplay7K.h"
#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K_Mechanics.h"

void VSRGMechanics::Setup(VSRG::Song *Song, VSRG::Difficulty *Difficulty, ScoreKeeper7K *scoreKeeper)
{
	CurrentSong = Song;
	CurrentDifficulty = Difficulty;
	score_keeper = scoreKeeper;
}

bool RaindropMechanics::OnUpdate(double SongTime, VSRG::TrackNote* m, uint32 Lane)
{
	uint32 k = Lane;
	/* We have to check for all gameplay conditions for this note. */

	// Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
	// note wasn't hit at the head and it's a hold
	if ((SongTime - m->GetTimeFinal()) > 0 && !m->WasNoteHit() && m->IsHold()) {
		// ^ no need for delays here.
		// remove hold notes that were never hit.
		MissNotify(abs(SongTime - m->GetTimeFinal()) * 1000, k, m->IsHold(), true, false);
		m->Hit();
	} // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
	else if ((SongTime - m->GetStartTime()) * 1000 > score_keeper->getMissCutoff() &&
		(!m->WasNoteHit() && m->IsHeadEnabled()))
	{
		MissNotify(abs(SongTime - m->GetStartTime()) * 1000, k, m->IsHold(), false, false);

		// only remove tap notes from judgment; hold notes might be activated before the tail later.
		if (!(m->IsHold())){
			m->Disable();
		}
		else{
			m->DisableHead();
			if (IsLaneKeyDown(k)){ // if the note was already being held down
				m->Hit();
				SetLaneHoldingState(k, true);
			}
		}

	} // Condition C: Hold head was hit, but hold tail was not released.
	else if ((SongTime - m->GetTimeFinal()) * 1000 > score_keeper->getMissCutoff() &&
		m->IsHold() && m->WasNoteHit() && m->IsEnabled())
	{
		MissNotify (abs(SongTime - m->GetTimeFinal()) * 1000, k, m->IsHold(), true, false);

		SetLaneHoldingState(k, false);
		m->Disable();
	} // Condition D: Hold head was hit, but was released early was already handled at ReleaseLane so no need to be redundant here.

	return false;
}

bool RaindropMechanics::OnPressLane(double SongTime, VSRG::TrackNote* m, uint32 Lane)
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
		else{
			m->Hit();
			HitNotify(dev, Lane, m->IsHold(), false);

			PlayNoteSoundEvent(m->GetSound());

			if (m->IsHold())
				SetLaneHoldingState(Lane, true);
			else
				m->Disable();
		}

		return true;
	}

	return false;
}

bool RaindropMechanics::OnReleaseLane(double SongTime, VSRG::TrackNote* m, uint32 Lane)
{
	if (m->IsHold() && m->WasNoteHit() && m->IsEnabled()) /* We hit the hold's head and we've not released it early already */
	{
		double dev = (SongTime - m->GetTimeFinal()) * 1000;
		double tD = abs(dev);

		if (tD < score_keeper->getJudgmentWindow(SKJ_W3)) /* Released in time */
		{
			HitNotify(dev, Lane, m->IsHold(), true);
			SetLaneHoldingState(Lane, false);
			m->Disable();
		}
		else /* Released off time */
		{
			// early misses for hold notes always count as regular misses.
			MissNotify(dev, Lane, m->IsHold(), false, false);

			m->Disable();
			SetLaneHoldingState(Lane, false);
		}
		return true;
	}

	return false;
}