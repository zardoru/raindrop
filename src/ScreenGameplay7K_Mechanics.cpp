#include "GameGlobal.h"
#include "Screen.h"
#include "Audio.h"
#include "ImageLoader.h"
#include "GameWindow.h"

#include "LuaManager.h"
#include "GraphObject2D.h"
#include "GraphObjectMan.h"
#include "ImageList.h"

#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K.h"

#include <iomanip>
#include <cmath>

using namespace VSRG;

void ScreenGameplay7K::RecalculateMatrix()
{
	PositionMatrix = glm::translate(Mat4(), glm::vec3(0, JudgmentLinePos + CurrentVertical * SpeedMultiplier, 0));
	PositionMatrixJudgment = glm::translate(Mat4(), glm::vec3(0, JudgmentLinePos, 0));

	for (uint8 i = 0; i < Channels; i++)
		NoteMatrix[i] = glm::translate(Mat4(), glm::vec3(LanePositions[i], 0, 14)) * noteEffectsMatrix[i] *  glm::scale(Mat4(), glm::vec3(LaneWidth[i], NoteHeight, 1));
}

void ScreenGameplay7K::RecalculateEffects()
{

	if (waveEffectEnabled)
	{
		float cs = sin (CurrentBeat * M_PI / 4);
		waveEffect = cs * 0.35 * min(SpeedMultiplierUser, 2.0f);
		MultiplierChanged = true;
	}

/*
		for (int i = 0; i < Channels; i++)
			noteEffectsMatrix[i] = glm::translate(glm::mat4(), glm::vec3(cos(CurrentBeat * PI / 4) * 30, 0, 0) ) / ** glm::rotate(glm::mat4(), 360 * CurrentBeat, glm::vec3(0,0,1) )*/;

	if (Upscroll)
		SpeedMultiplier = - (SpeedMultiplierUser + waveEffect + beatScrollEffect);
	else
		SpeedMultiplier = SpeedMultiplierUser + waveEffect + beatScrollEffect;
}

void ScreenGameplay7K::UpdateScriptScoreVariables()
{

	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	L->SetGlobal("MaxCombo", score_keeper->getScore(ST_MAX_COMBO));
	L->SetGlobal("Accuracy", score_keeper->getPercentScore(PST_ACC));
	L->SetGlobal("SCScore", score_keeper->getScore(scoring_type));
	L->SetGlobal("EXScore", score_keeper->getScore(ST_EX));

	std::pair<std::string, int> autopacemaker = score_keeper->getAutoPacemaker();
	L->SetGlobal("PacemakerText", autopacemaker.first);
	L->SetGlobal("PacemakerValue", autopacemaker.second);

	L->SetGlobal("AccText", "ACC:");
	L->SetGlobal("AccValue", score_keeper->getPercentScore(PST_EX));

	double lifebar_amount = score_keeper->getLifebarAmount(lifebar_type);
	L->SetGlobal("LifebarValue", lifebar_amount);
	if(lifebar_type == LT_GROOVE || lifebar_type == LT_EASY)
		L->SetGlobal("LifebarDisplay", max(2, int(floor(lifebar_amount * 50) * 2)));
	else
		L->SetGlobal("LifebarDisplay", int(ceil(lifebar_amount * 50) * 2));

}


void ScreenGameplay7K::HitNote (double TimeOff, uint32 Lane, bool IsHold, bool IsHoldRelease)
{
	int Judgment = score_keeper->hitNote(TimeOff);

	UpdateScriptScoreVariables();

	Animations->GetEnv()->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	Animations->GetEnv()->CallFunction("HitEvent", 5);
	Animations->GetEnv()->PushArgument(Judgment);
	Animations->GetEnv()->PushArgument(TimeOff);
	Animations->GetEnv()->PushArgument((int)Lane + 1);
	Animations->GetEnv()->PushArgument(IsHold);
	Animations->GetEnv()->PushArgument(IsHoldRelease);
	Animations->GetEnv()->RunFunction();

	if (score_keeper->getMaxNotes() == score_keeper->getScore(ST_NOTES_HIT))
		Animations->DoEvent("OnFullComboEvent");
}

void ScreenGameplay7K::MissNote (double TimeOff, uint32 Lane, bool IsHold, bool auto_hold_miss, bool early_miss)
{

	score_keeper->missNote(auto_hold_miss, early_miss);

	if (IsHold)
		HeldKey[Lane] = false;

	// 3 seconds showing miss bga
	MissTime = 3;

	UpdateScriptScoreVariables();

	Animations->GetEnv()->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	Animations->GetEnv()->CallFunction("MissEvent", 3);
	Animations->GetEnv()->PushArgument(TimeOff);
	Animations->GetEnv()->PushArgument((int)Lane + 1);
	Animations->GetEnv()->PushArgument(IsHold);
	Animations->GetEnv()->RunFunction();

}

void ScreenGameplay7K::RunMeasures()
{
	double timeClosest[VSRG::MAX_CHANNELS];

	for (int i = 0; i < VSRG::MAX_CHANNELS; i++)
		timeClosest[i] = CurrentDiff->Duration;

	for (uint16 k = 0; k < Channels; k++)
	{
		for (std::vector<TrackNote>::iterator m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); m++)	{

			// Keysound update to closest note.
			if (CurrentDiff->IsVirtual)	{
				if (m->IsEnabled())
				{
					if ((abs(SongTime - m->GetTimeFinal()) < timeClosest[k]))
					{
						PlaySounds[k] = m->GetSound();
						timeClosest[k] = abs(SongTime - m->GetTimeFinal());
					}else
						break; // In other words, we're getting further away.
				}
			}


			if (Auto) {
				float TimeThreshold = SongTime + 0.016;
				if ( m->GetStartTime() < TimeThreshold) // allow a tolerance equal to the judgment window?
				{
					if (m->IsEnabled()) {
						if (m->IsHold())
						{
							if (m->WasNoteHit())
							{
								if ( m->GetTimeFinal() < TimeThreshold)
									ReleaseLane(k, SongTime);
									// ReleaseLane(k, m->GetTimeFinal());
							}else
								JudgeLane(k, SongTime);
								// JudgeLane(k, m->GetStartTime());
						}else
						{
							JudgeLane(k, SongTime);
							ReleaseLane(k, SongTime);
							// JudgeLane(k, m->GetStartTime());
							// ReleaseLane(k, m->GetTimeFinal());
						}
					}
				}
			}


			if(stage_failed) continue; // don't check for judgments after stage is failed.


			/* We have to check for all gameplay conditions for this note. */

			// Condition A: Hold tail outside accuracy cutoff (can't be hit any longer),
			// note wasn't hit at the head and it's a hold
			if ((SongTime - m->GetTimeFinal()) * 1000 > score_keeper->getMissCutoff() && !m->WasNoteHit() && m->IsHold()) {

				// remove hold notes that were never hit.
				MissNote(abs(SongTime - m->GetTimeFinal()) * 1000, k, m->IsHold(), true, false);

				if (score_keeper->getScore(ST_COMBO) > 10)
					MissSnd->Play();

				m->Hit();

			} // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
			else if ((SongTime - m->GetStartTime()) * 1000 > score_keeper->getMissCutoff() &&
				(!m->WasNoteHit() && m->IsEnabled()))
			{

				MissNote(abs(SongTime - m->GetStartTime()) * 1000, k, m->IsHold(), false, false);

				if (score_keeper->getScore(ST_COMBO) > 10)
					MissSnd->Play();

				/* remove note from judgment
				relevant to note that hit isn't being set here so the hold tail can be judged later. */
				m->Disable();

			} // Condition C: Hold head was hit, but hold tail was not released.
			else if ((SongTime - m->GetTimeFinal()) * 1000 > score_keeper->getMissCutoff() &&
				m->IsHold() && m->WasNoteHit() && m->IsEnabled())
			{

				MissNote(abs(SongTime - m->GetTimeFinal()) * 1000, k, m->IsHold(), true, false);

				HeldKey[k] = false;
				m->Disable();

			} // Condition D: Hold head was hit, but was released early was already handled at ReleaseLane so no need to be redundant here.

		} // end for notes
	} // end for channels
}

void ScreenGameplay7K::ReleaseLane(uint32 Lane, float Time)
{
	GearKeyEvent(Lane, false);

	if(stage_failed) return; // don't judge any more after stage is failed.

	for (std::vector<TrackNote>::iterator m = NotesByChannel[Lane].begin(); m != NotesByChannel[Lane].end(); m++)
	{
		if (m->IsHold() && m->WasNoteHit() && m->IsEnabled()) /* We hit the hold's head and we've not released it early already */
		{

			double dev = (Time - m->GetTimeFinal()) * 1000;
			double tD = abs(dev);

			if (tD < score_keeper->getJudgmentWindow(SKJ_W3)) /* Released in time */
			{
				HitNote(dev, Lane, m->IsHold(), true);

				HeldKey[Lane] = false;

				m->Disable();

			}else /* Released off time */
			{
				// early misses for hold notes always count as regular misses.
				MissNote(dev, Lane, m->IsHold(), false, false);

				if (score_keeper->getScore(ST_COMBO) > 10)
					MissSnd->Play();

				m->Disable();
				HeldKey[Lane] = false;
			}

			lastClosest[Lane] = (double)min(tD, (double)lastClosest[Lane]);
			return;
		}
	}
}

void ScreenGameplay7K::JudgeLane(uint32 Lane, float Time)
{
	float MsDisplayMargin = (Configuration::GetSkinConfigf("HitErrorDisplayLimiter"));

	if ( (!Music && !CurrentDiff->IsVirtual) || !Active || stage_failed)
		return;

	GearKeyEvent(Lane, true);

	lastClosest[Lane] = MsDisplayMargin;

	for (std::vector<TrackNote>::iterator m = NotesByChannel[Lane].begin(); m != NotesByChannel[Lane].end(); m++)
	{
		if (!m->IsEnabled())
			continue;

		double dev = (Time - m->GetStartTime()) * 1000;
		double tD = abs(dev);

		lastClosest[Lane] = min(tD, (double)lastClosest[Lane]);

		if (lastClosest[Lane] >= MsDisplayMargin)
			lastClosest[Lane] = 0;

		if (tD > score_keeper->getEarlyMissCutoff()) // If the note was hit outside of judging range
		{

			// do nothing else
			if (PlaySounds[Lane])
			{
				if (Keysounds[PlaySounds[Lane]])
					Keysounds[PlaySounds[Lane]]->Play();
			}

			continue;

		}

		else // Within judging range, including early misses
		{

			// early miss
			if(dev < -(score_keeper->getMissCutoff())){

				// treat specially for Beatmania.

				MissSnd->Play();

				MissNote(dev, Lane, m->IsHold(), m->IsHold(), true);
				// missed feedback
				// m->Disable();

			}else{

				m->Hit();
				HitNote(dev, Lane, m->IsHold());

				if (m->GetSound())
				{
					if (Keysounds[m->GetSound()] && PlayReactiveSounds)
						Keysounds[m->GetSound()]->Play();
				}

				if (m->IsHold())
					HeldKey[Lane] = true;
				else
					m->Disable();

			}

			return; // we judged a note in this lane, so we're done.
		}
	}
}

void ScreenGameplay7K::UpdateScriptVariables()
{
	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("SpeedMultiplier", SpeedMultiplier);
	L->SetGlobal("SpeedMultiplierUser", SpeedMultiplierUser);
	L->SetGlobal("waveEffectEnabled", waveEffectEnabled);
	L->SetGlobal("Active", Active);
	L->SetGlobal("SongTime", SongTime);

	CurrentBeat = IntegrateToTime(CurrentDiff->BPS, SongTime);
	L->SetGlobal("Beat", CurrentBeat);

	L->NewArray();

	for (uint32 i = 0; i < Channels; i++)
	{
		L->SetFieldI(i + 1, HeldKey[i]);
	}

	L->FinalizeArray("HeldKeys");

	L->SetGlobal("CurrentSPB", 1 / SectionValue(CurrentDiff->BPS, SongTime));
	L->SetGlobal("CurrentBPM", 60 * SectionValue(CurrentDiff->BPS, SongTime));
}
