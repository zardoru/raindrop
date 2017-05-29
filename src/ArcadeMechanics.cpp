#include "pch.h"

#include "TrackNote.h"
#include "VSRGMechanics.h"
#include "ScoreKeeper7K.h"

namespace Game {
	namespace VSRG {
		int RaindropArcadeMechanics::GetScratchForLane(uint32_t Lane)
		{
			if (Lane == SCRATCH_1P_CHANNEL) {
				return 0;
			}
			else if (Lane == SCRATCH_2P_CHANNEL) {
				return 1;
			}
			else {
				throw std::runtime_error("Scratch called on non-scratch lane!");
			}

			return 0;
		}
		bool RaindropArcadeMechanics::CanHitNoteHead(double time, TrackNote * note)
		{
			return (abs(time - note->GetStartTime()) < score_keeper->getJudgmentCutoff()) && note->IsHeadEnabled() && !note->WasHit();
		}
		bool RaindropArcadeMechanics::CanHitNoteTail(double time, TrackNote * note)
		{
			return !note->IsHeadEnabled() && note->WasHit();
		}
		void RaindropArcadeMechanics::JudgeScratch(double SongTime, TrackNote* Note, uint32_t Lane, EScratchState newScratchState, EScratchState oldScratchState)
		{

			if ( (newScratchState != SCR_NEUTRAL && oldScratchState == SCR_NEUTRAL) ||
				 (oldScratchState == SCR_UP && newScratchState == SCR_DOWN) ||
				 (oldScratchState == SCR_DOWN && newScratchState == SCR_UP)) {

				PerformJudgement(SongTime, Note, Lane);
			}
		}

		void RaindropArcadeMechanics::PerformJudgement(double SongTime, TrackNote * Note, uint32_t Lane)
		{
			// From neutral or opposite scratch, or key press, trigger the head.
			double dev = 1000. * (SongTime - Note->GetStartTime());
			if (IsEarlyMiss(SongTime, Note)) {
				MissNotify(dev, Lane, Note->IsHold(), false, true);
			}
			else {
				// Heads, Non-holds
				if (Note->IsHeadEnabled() && !Note->WasHit()) {
					// Within miss judgement?
					if (!IsBmBadJudge(SongTime, Note)) {
						// Hit head
						Note->Hit();
						HitNotify(dev, Lane, Note->IsHold(), false);

						PlayNoteSoundEvent(Note->GetSound());

						if (Note->IsHold()) {
							SetLaneHoldingState(Lane, true);
							Note->DisableHead();
						} else {
							Note->Disable();
							Note->MakeInvisible(); // Should we do this?
						}

					} else {
						// Completely disable head or note
						Note->FailHit();

						if (Note->IsHold())
							Note->DisableHead();
						else
							Note->Disable();

						MissNotify(dev, Lane, Note->IsHold(), false, false);
					}
				} else { // Hold Tails
					
					Note->Disable();

					double tdev = (Note->GetEndTime() - SongTime) * 1000.;
					// Tail is within judge window, and head was hit
					if ( abs(tdev) < score_keeper->getJudgmentWindow(SKJ_W3)
						&& Note->WasHit()) {
						Note->Hit();
						HitNotify(tdev, Lane, Note->IsHold(), true);
					} else { // Tail outside judgement
						Note->FailHit();
						MissNotify(dev, Lane, Note->IsHold(), false, false);
					}

					SetLaneHoldingState(Lane, false);
				}
			}
		}

		RaindropArcadeMechanics::RaindropArcadeMechanics()
		{
			ScratchState[0] = ScratchState[1] = SCR_NEUTRAL;
		}

		bool RaindropArcadeMechanics::OnUpdate(double SongTime, TrackNote * Note, uint32_t Lane)
		{
			if (!Note->IsEnabled()) return false;

			double miss_time = score_keeper->getJudgmentWindow(SKJ_W3);
			double dev = (SongTime - Note->GetStartTime()) * 1000.;
			double tail_dev = (SongTime - Note->GetEndTime()) * 1000.;
			
			if ( (dev > miss_time && Note->IsHeadEnabled()) ||  // Judge head only if not hit or regular note 
					(tail_dev > miss_time && !Note->IsHeadEnabled()) ) { // Judge tail regardless of whether it was hit or not
				
				Note->FailedHit();

				// Check for nonhold or deactivated head
				if (!Note->IsHold() || !Note->IsHeadEnabled()) {
					Note->Disable();

					if (Note->WasHit() && Note->IsHold()) {
						SetLaneHoldingState(Lane, false);
					}
				}

				// Check hold with activated head
				if (Note->IsHold() && Note->IsHeadEnabled())
					Note->DisableHead();

				// Will "emergingly" miss head and tail at their respective times.

				MissNotify(dev, Lane, Note->IsHold(), false, false);
			}

			return false;
		}
		bool RaindropArcadeMechanics::OnPressLane(double SongTime, TrackNote * Note, uint32_t Lane)
		{
			if (!Note->IsEnabled()) return false;
			if (!InJudgeCutoff(SongTime, Note)) return false;
			if (!CanHitNoteHead(SongTime, Note)) return false;

			PerformJudgement(SongTime, Note, Lane);
			return true;
		}
		bool RaindropArcadeMechanics::OnReleaseLane(double SongTime, TrackNote * Note, uint32_t Lane)
		{
			if (!Note->IsEnabled()) return false;
			if (!InJudgeCutoff(SongTime, Note)) return false;
			if (!CanHitNoteTail(SongTime, Note)) return false;

			PerformJudgement(SongTime, Note, Lane);
			return true;
		}
		bool RaindropArcadeMechanics::OnScratchUp(double SongTime, TrackNote * Note, uint32_t Lane)
		{
			if (!Note->IsEnabled()) return false;
			int scratch = GetScratchForLane(Lane);
			bool judgeHead = CanHitNoteHead(SongTime, Note);
			bool judgeTail = CanHitNoteTail(SongTime, Note);
			if (!judgeHead && !judgeTail) return false;

			JudgeScratch(SongTime, Note, Lane, SCR_UP, ScratchState[scratch]);
			ScratchState[scratch] = SCR_UP;
			return true;
		}
		bool RaindropArcadeMechanics::OnScratchDown(double SongTime, TrackNote * Note, uint32_t Lane)
		{
			if (!Note->IsEnabled()) return false;
			int scratch = GetScratchForLane(Lane);
			bool judgeHead = CanHitNoteHead(SongTime, Note);
			bool judgeTail = CanHitNoteTail(SongTime, Note);
			if (!judgeHead && !judgeTail) return false;

			JudgeScratch(SongTime, Note, Lane, SCR_DOWN, ScratchState[scratch]);
			ScratchState[scratch] = SCR_DOWN;
			return true;
		}
		bool RaindropArcadeMechanics::OnScratchNeutral(double SongTime, TrackNote * Note, uint32_t Lane)
		{
			if (!Note->IsEnabled()) return false;
			int scratch = GetScratchForLane(Lane);
			bool judgeHead = CanHitNoteHead(SongTime, Note);
			bool judgeTail = CanHitNoteTail(SongTime, Note);
			if (!judgeHead && !judgeTail) return false;

			if (ScratchState[scratch] != SCR_NEUTRAL) {
				// It's an active hold? Then kill it.
				if (Note->WasHit() && !Note->IsHeadEnabled()) {
					Note->FailHit();
					Note->Disable();
				}
			} // else do nothing

			ScratchState[scratch] = SCR_NEUTRAL;
			return true;
		}
		TimingType RaindropArcadeMechanics::GetTimingKind()
		{
			return TT_TIME;
		}
	}
}