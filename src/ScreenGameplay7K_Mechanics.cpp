#include "GameGlobal.h"
#include "Screen.h"
#include "Audio.h"
#include "FileManager.h"
#include "ImageLoader.h"
#include "GameWindow.h"

#include "LuaManager.h"
#include "GraphObject2D.h"
#include "GraphObjectMan.h"

#include "ScoreKeeper.h"
#include "ScreenGameplay7K.h"

using namespace VSRG;

void ScreenGameplay7K::RecalculateEffects()
{
	float SongTime = 0;

	if (Music)
		SongTime = Music->GetPlayedTime();

	if (waveEffectEnabled)
	{
		float cs = sin (CurrentBeat * M_PI / 4);
		waveEffect = cs * 0.35 * min(SpeedMultiplierUser, 2.0f);
		MultiplierChanged = true;
	}

/*
		for (int i = 0; i < Channels; i++)
			noteEffectsMatrix[i] = glm::translate(glm::mat4(), glm::vec3(cos(CurrentBeat * PI / 4) * 30, 0, 0) ) /** glm::rotate(glm::mat4(), 360 * CurrentBeat, glm::vec3(0,0,1) )*/;

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
	L->SetGlobal("EXScore", score_keeper->getPercentScore(PST_EX));
	L->SetGlobal("Lifebar", score_keeper->getLifebarAmount(LT_GROOVE));
}


void ScreenGameplay7K::HitNote (double TimeOff, uint32 Lane, bool IsHold, bool IsHoldRelease)
{
	int Judgement = score_keeper->hitNote(TimeOff);

	UpdateScriptScoreVariables();

	Animations->GetEnv()->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	Animations->GetEnv()->CallFunction("HitEvent", 5);
	Animations->GetEnv()->PushArgument(Judgement);
	Animations->GetEnv()->PushArgument(TimeOff);
	Animations->GetEnv()->PushArgument((int)Lane + 1);
	Animations->GetEnv()->PushArgument(IsHold);
	Animations->GetEnv()->PushArgument(IsHoldRelease);
	Animations->GetEnv()->RunFunction();
}

void ScreenGameplay7K::MissNote (double TimeOff, uint32 Lane, bool IsHold, bool auto_hold_miss)
{
	score_keeper->missNote(auto_hold_miss);

	if (IsHold)
		HeldKey[Lane] = false;

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
		MeasureVectorTN &Measures = NotesByMeasure[k];

		for (MeasureVectorTN::iterator i = Measures.begin(); i != Measures.end(); i++) {
			for (std::vector<TrackNote>::iterator m = (*i).begin(); m != (*i).end(); m++)	{

				// Keysound update to closest note.
				if (CurrentDiff->IsVirtual)	{
					if (m->IsEnabled() && (abs(SongTime - m->GetTimeFinal()) < timeClosest[k]))	{
						PlaySounds[k] = m->GetSound();
						timeClosest[k] = abs(SongTime - m->GetTimeFinal());
					}
				}


				/* We have to check for all gameplay conditions for this note. */

				// Condition A: Hold tail outside accuracy cutoff (can't be hit any longer), 
				// note wasn't hit at the head and it's a hold
				if ((SongTime - m->GetTimeFinal()) * 1000 > score_keeper->getAccCutoff() && !m->WasNoteHit() && m->IsHold()) {

					// remove hold notes that were never hit.
					MissNote(abs(SongTime - m->GetTimeFinal()) * 1000, k, m->IsHold(), true);

					if (score_keeper->getScore(ST_COMBO) > 10)
						MissSnd->Play();

					m->Hit();

				} // Condition B: Regular note or hold head outside cutoff, wasn't hit and it's enabled.
				else if ((SongTime - m->GetStartTime()) * 1000 > score_keeper->getAccCutoff() && 
					(!m->WasNoteHit() && m->IsEnabled()))
				{
					MissNote(abs(SongTime - m->GetStartTime()) * 1000, k, m->IsHold(), false);

					if (score_keeper->getScore(ST_COMBO) > 10)
						MissSnd->Play();

					/* remove note from judgement
					   relevant to note that hit isn't being set here so the hold tail can be judged later.	*/
					m->Disable();
				} // Condition C: Hold head was hit, but hold tail was not released.
				else if ((SongTime - m->GetTimeFinal()) * 1000 > score_keeper->getAccCutoff() &&
					m->IsHold() && m->WasNoteHit() && m->IsEnabled())
				{
					MissNote(abs(SongTime - m->GetTimeFinal()) * 1000, k, m->IsHold(), true);

					HeldKey[k] = false;
					m->Disable();
				}

			} // end for notes
		} // end for measures
	} // end for channels
}

void ScreenGameplay7K::ReleaseLane(uint32 Lane)
{
	MeasureVectorTN &Measures = NotesByMeasure[Lane];

	for (MeasureVectorTN::iterator i = Measures.begin(); i != Measures.end(); i++)
	{
		for (std::vector<TrackNote>::iterator m = (*i).begin(); m != (*i).end(); m++)
		{
			if (m->WasNoteHit() && m->IsEnabled()) /* We hit the hold's head and we've not released it early already */
			{
				double tD = abs (m->GetTimeFinal() - SongTime) * 1000;

				if (tD < score_keeper->getAccCutoff()) /* Released in time */
				{
					HitNote(tD, Lane, m->IsHold(), true);

					HeldKey[Lane] = false;

					m->Disable();

				}else /* Released off time */
				{
					MissNote(tD, Lane, m->IsHold(), false);

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
}

void ScreenGameplay7K::JudgeLane(uint32 Lane)
{
	float MsDisplayMargin = (Configuration::GetSkinConfigf("HitErrorDisplayLimiter"));
	MeasureVectorTN &Measures = NotesByMeasure[Lane];

	if (!Music || !Active)
		return;

	lastClosest[Lane] = MsDisplayMargin;

	for (MeasureVectorTN::iterator i = Measures.begin(); i != Measures.end(); i++)
	{
		for (std::vector<TrackNote>::iterator m = (*i).begin(); m != (*i).end(); m++)
		{
			if (!m->IsEnabled())
				continue;

			double tD = abs (m->GetStartTime() - SongTime) * 1000;
			// std::cout << "\n time: " << m->GetStartTime() << " st: " << SongTime << " td: " << tD;

			lastClosest[Lane] = min(tD, (double)lastClosest[Lane]);

			if (lastClosest[Lane] >= MsDisplayMargin)
			{
				lastClosest[Lane] = 0;
			}

			if (tD > score_keeper->getAccCutoff())
			{
				if (PlaySounds[Lane])
				{
					if (Keysounds[PlaySounds[Lane]])
						Keysounds[PlaySounds[Lane]]->Play();
				}

				continue;
			}
			else
			{
				if (tD <= score_keeper->getAccMax())
				{
					m->Hit();
					HitNote(tD, Lane, m->IsHold());

					if (m->GetSound())
					{
						if (Keysounds[m->GetSound()])
							Keysounds[m->GetSound()]->Play();
					}

					if (m->IsHold())
						HeldKey[Lane] = true;
					else
						m->Disable();
				}
				else
				{
					MissNote(tD, Lane, m->IsHold(), m->IsHold());
					// missed feedback
					MissSnd->Play();
					m->Disable();
				}

				return; // we judged a note in this lane, so we're done.
			}
		}
	}
}
