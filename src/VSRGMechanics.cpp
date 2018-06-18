#include "pch.h"

#include "Logging.h"

#include "GameGlobal.h"
#include "Screen.h"
#include "Sprite.h"
#include "ImageList.h"

#include "ScreenGameplay7K.h"
#include "ScoreKeeper7K.h"
#include "VSRGMechanics.h"
#include "TrackNote.h"
#include "NoteTransformations.h"

namespace Game {
	namespace VSRG {
		bool Mechanics::IsLateHeadMiss(double t, TrackNote * note)
		{
			return (t - note->GetStartTime()) * 1000.0 > PlayerScoreKeeper->getLateMissCutoffMS();
		}

		bool Mechanics::InJudgeCutoff(double t, TrackNote * note)
		{
			double earlyMissCutoff = PlayerScoreKeeper->getEarlyMissCutoffMS() / 1000.0;
			double missCutoff = PlayerScoreKeeper->getLateMissCutoffMS() / 1000.0;
			return (abs(t - note->GetStartTime()) <= earlyMissCutoff) ||
				(abs(t - note->GetEndTime()) <= missCutoff);
		}
		bool Mechanics::IsEarlyMiss(double t, TrackNote * note)
		{
			double dt = (t - note->GetStartTime()) * 1000.;
			return dt < -PlayerScoreKeeper->getLateMissCutoffMS() && dt >= -PlayerScoreKeeper->getEarlyMissCutoffMS();
		}

		bool Mechanics::IsBmBadJudge(double t, TrackNote * note)
		{
			double dt = abs(t - note->GetStartTime()) * 1000.0;
			return dt > PlayerScoreKeeper->getJudgmentWindow(SKJ_W3) && dt < PlayerScoreKeeper->getJudgmentWindow(SKJ_W4);
		}

		bool Mechanics::InHeadCutoff(double t, TrackNote * note)
		{
			double dev = (t - note->GetStartTime()) * 1000;
			return dev >= -PlayerScoreKeeper->getEarlyMissCutoffMS() && 
				dev <= PlayerScoreKeeper->getLateMissCutoffMS();
		}
		

		void Mechanics::TransformNotes(PlayerChartState & ChartState)
		{
			if (GetTimingKind() == TT_BEATS) {
				NoteTransform::TransformToBeats(
					CurrentDifficulty->Channels,
					ChartState.Notes, 
					ChartState.BPS);
			}
		}

		void Mechanics::Setup(VSRG::Difficulty *Difficulty, std::shared_ptr<ScoreKeeper> scoreKeeper)
		{
			CurrentDifficulty = Difficulty;
			PlayerScoreKeeper = scoreKeeper;
		}

		RaindropMechanics::RaindropMechanics(bool forcedRelease)
		{
			this->forcedRelease = forcedRelease;
		}

		bool RaindropMechanics::OnUpdate(double SongTime, TrackNote* m, uint32_t Lane)
		{
			auto k = Lane;
			/* We have to check for all gameplay conditions for this note. */
			double missCutoff = PlayerScoreKeeper->getLateMissCutoffMS();

			// Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
			// note wasn't hit at the head and can't be hit at the head, and it's a hold
			if (!InHeadCutoff(SongTime, m) // head outside judgment
				&& !m->WasHit() && m->IsHold()) // not hit yet
			{
				double dev = (SongTime - m->GetEndTime()) * 1000;
				double tD = abs(dev);
				
				if (dev > 0) {
					// remove hold notes that were never hit.
					m->MakeInvisible();

					if (MissNotify)
						MissNotify(tD, k, m->IsHold(), true, false);

					m->Hit();

					return true;
				}

			} // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
			else if (IsLateHeadMiss(SongTime, m) &&
				(!m->WasHit() && m->IsHeadEnabled()))
			{
				if (MissNotify)
					MissNotify(
						abs(SongTime - m->GetStartTime()) * 1000, 
						k, 
						m->IsHold(), 
						false, 
						false
					);

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

						if (SetLaneHoldingState)
							SetLaneHoldingState(k, true);
					}
				}

				return true;
			} // Condition C: Hold head was hit, but hold tail was not released.
			else if (m->IsHold() && m->IsEnabled() && m->WasHit())
			{
				// Condition C-1: Forced release is enabled
				if ((SongTime - m->GetEndTime()) * 1000 > missCutoff && forcedRelease)
				{
					m->FailHit();
					// Take away health and combo (1st false)

					if (MissNotify)
						MissNotify(abs(SongTime - m->GetEndTime()) * 1000, k, m->IsHold(), false, false);

					if (SetLaneHoldingState)
						SetLaneHoldingState(k, false);

					m->Disable();

					return true;
				}
				else
				{
					// Condition C-2: Forced release is not enabled
					if (SongTime - m->GetEndTime() > 0 && 
						!forcedRelease)
					{
						if (IsLaneKeyDown(Lane))
						{
							if (HitNotify)
								HitNotify(0, k, true, true);
						}
						else
						{
							// Only take away health, but not combo (1st true)
							if (MissNotify)
								MissNotify(
									PlayerScoreKeeper->getLateMissCutoffMS(), 
									k, 
									m->IsHold(), 
									true, 
									false
								);
						}

						if (SetLaneHoldingState)
							SetLaneHoldingState(k, false);

						m->Disable();
						return true;
					}
				}
			} // Condition D: Hold head was hit, but was released early was already handled at ReleaseLane so no need to be redundant here.

			return false;
		}

		bool RaindropMechanics::OnPressLane(double SongTime, TrackNote* m, uint32_t Lane)
		{
			if (!m->IsEnabled())
				return false;

			double dev = (SongTime - m->GetStartTime()) * 1000;
			double tD = abs(dev);

			if (!InHeadCutoff(SongTime, m)) // If the note was hit outside of judging range
			{
				// Log::Printf("td > jc %f %f\n", tD, score_keeper->getJudgmentCutoff());
				// do nothing else for this note - anyway, this case happens if note optimization is disabled.
				return false;
			}
			else // Within judging range, including early misses
			{
				// early miss
				if (IsEarlyMiss(SongTime, m))
				{
					if (MissNotify)
						MissNotify(dev, Lane, m->IsHold(), m->IsHold(), true);
				}
				else
				{
					m->Hit();
					if (HitNotify)
						HitNotify(dev, Lane, m->IsHold(), false);

					if (m->IsHold())
					{
						if (SetLaneHoldingState)
							SetLaneHoldingState(Lane, true);
					}
					else
					{
						m->Disable();
						m->MakeInvisible();
					}
				}

				if (PlayNoteSoundEvent)
					PlayNoteSoundEvent(m->GetSound());

				return true;
			}

			return false;
		}

		bool RaindropMechanics::OnReleaseLane(double SongTime, TrackNote* m, uint32_t Lane)
		{
			if (m->IsHold() && m->WasHit() && m->IsEnabled()) /* We hit the hold's head and we've not released it early already */
			{
				double dev = (SongTime - m->GetEndTime()) * 1000;
				double tD = abs(dev);

				double earlyHit = PlayerScoreKeeper->getEarlyMissCutoffMS();
				double lateMiss = PlayerScoreKeeper->getLateMissCutoffMS();

				double releaseWindow;

				if (forcedRelease) {
					releaseWindow = PlayerScoreKeeper->getJudgmentWindow(SKJ_W3);
				} else
					releaseWindow = 250; // 250 ms

				/* Released in time */
				if ( ((tD < releaseWindow) && !forcedRelease) ||
					 (dev > -earlyHit && dev < lateMiss) )
				{
					// Only consider it a timed thing if releasing it is forced.
					if (HitNotify)
						HitNotify(forcedRelease ? dev : 0, Lane, true, true);
					
					if (SetLaneHoldingState)
						SetLaneHoldingState(Lane, false);

					m->Disable();
				}
				else /* Released off time */
				{
					// early misses for hold notes always count as regular misses.
					// they don't break combo when we're not doing forced releases.
					m->FailHit();
					
					if (MissNotify)
						MissNotify(dev, Lane, true, false, false);

					m->Disable();

					if (SetLaneHoldingState)
						SetLaneHoldingState(Lane, false);
				}

				if (PlayNoteSoundEvent)
					PlayNoteSoundEvent(m->GetTailSound());

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
			if (m->IsHold() && m->WasHit() && m->IsEnabled()) /* We hit the hold's head and we've not released it early already */
			{
				double dev = (SongBeat - m->GetEndTime());
				double tD = abs(dev);

				if (tD < PlayerScoreKeeper->getJudgmentWindow(SKJ_W3)) /* Released in time */
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

				PlayNoteSoundEvent(m->GetTailSound());
				
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

			if (tD < PlayerScoreKeeper->getJudgmentWindow(SKJ_W3)) // If the note was hit inside judging range
			{
				m->Hit();

				HitNotify(dev, Lane, m->IsHold(), false);

				if (m->IsHold())
					SetLaneHoldingState(Lane, true);
				else
				{
					m->Disable();

					// BADs stay visible.
					if (tD < PlayerScoreKeeper->getJudgmentWindow(SKJ_W2))
						m->MakeInvisible();
				}

				PlayNoteSoundEvent(m->GetSound());

				return true;
			}
			else if (tD > PlayerScoreKeeper->getJudgmentWindow(SKJ_W3) && tD < PlayerScoreKeeper->getLateMissCutoffMS()) {
				m->FailHit();
				m->Disable();

				MissNotify(dev, Lane, m->IsHold(), false, false);
				PlayNoteSoundEvent(m->GetSound());
			}

			return false;
		}

		bool O2JamMechanics::OnUpdate(double SongBeat, VSRG::TrackNote* m, uint32_t Lane)
		{
			auto k = Lane;
			double tTail = SongBeat - m->GetEndTime();
			double tHead = SongBeat - m->GetStartTime();

			if (!m->IsEnabled()) return false; // keep looking

			// Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
			// note wasn't hit at the head and it's a hold
			if (tTail > 0 && !m->WasHit() && m->IsHold())
			{
				// remove hold notes that were never hit.
				m->FailHit();
				MissNotify(abs(tTail), k, m->IsHold(), true, false);
				m->Disable();
			} // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
			else if (tHead > PlayerScoreKeeper->getJudgmentWindow(SKJ_W3) && !m->WasHit() && m->IsEnabled())
			{
				m->FailHit();
				MissNotify(abs(tHead), k, m->IsHold(), false, false);

				// remove from judgment completely
				m->Disable();
			} // Condition C: Hold head was hit, but hold tail was not released.
			else if (tTail > PlayerScoreKeeper->getJudgmentWindow(SKJ_W3) &&
				m->IsHold() && m->WasHit() && m->IsEnabled())
			{
				m->FailHit();
				MissNotify(abs(tTail), k, m->IsHold(), false, false);

				SetLaneHoldingState(k, false);
				m->Disable();
			}

			return false;
		}
		bool Mechanics::OnScratchUp(double SongTime, VSRG::TrackNote * Note, uint32_t Lane)
		{
			return false;
		}
		bool Mechanics::OnScratchDown(double SongTime, VSRG::TrackNote * Note, uint32_t Lane)
		{
			return false;
		}
		bool Mechanics::OnScratchNeutral(double SongTime, VSRG::TrackNote * Note, uint32_t Lane)
		{
			return false;
		}
}
}