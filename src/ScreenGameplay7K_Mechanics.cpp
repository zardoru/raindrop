#include <stdio.h>

#include "Global.h"
#include "Screen.h"
#include "Configuration.h"
#include "Audio.h"
#include "FileManager.h"
#include "ImageLoader.h"
#include "Song.h"
#include "BitmapFont.h"
#include "GameWindow.h"

#include <iomanip>

#include "LuaManager.h"
#include "GraphObjectMan.h"

#include "ScoreKeeper.h"
#include "ScreenGameplay7K.h"

typedef std::vector<SongInternal::Measure7K> NoteVector;

void ScreenGameplay7K::RecalculateEffects()
{
	float SongTime = 0;

	if (Music)
		SongTime = Music->GetPlayedTime();

	if (waveEffectEnabled)
	{
		waveEffect = sin(SongTime) * 0.5 * SpeedMultiplierUser;
		MultiplierChanged = true;
	}

	if (Upscroll)
		SpeedMultiplier = - (SpeedMultiplierUser + waveEffect);
	else
		SpeedMultiplier = SpeedMultiplierUser + waveEffect;
}

void ScreenGameplay7K::UpdateScriptScoreVariables()
{
	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	L->SetGlobal("MaxCombo", score_keeper->getScore(ST_MAX_COMBO));
	L->SetGlobal("Accuracy", score_keeper->getPercentScore(PST_ACC));
	L->SetGlobal("EXScore", score_keeper->getPercentScore(PST_EX));
}


void ScreenGameplay7K::HitNote (double TimeOff, uint32 Lane)
{
	score_keeper->hitNote(TimeOff);

	UpdateScriptScoreVariables();

	Animations->GetEnv()->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	Animations->GetEnv()->CallFunction("HitEvent", 2);
	Animations->GetEnv()->PushArgument(TimeOff);
	Animations->GetEnv()->PushArgument((int)Lane + 1);
	Animations->GetEnv()->RunFunction();
}

void ScreenGameplay7K::MissNote (double TimeOff, uint32 Lane, bool auto_hold_miss)
{
	score_keeper->missNote(auto_hold_miss);

	UpdateScriptScoreVariables();

	Animations->GetEnv()->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	Animations->GetEnv()->CallFunction("MissEvent", 2);
	Animations->GetEnv()->PushArgument(TimeOff);
	Animations->GetEnv()->PushArgument((int)Lane + 1);
	Animations->GetEnv()->RunFunction();
}



void ScreenGameplay7K::RunMeasures()
{

	for (unsigned int k = 0; k < Channels; k++)
	{
		NoteVector &Measures = NotesByMeasure[k];

		for (NoteVector::iterator i = Measures.begin(); i != Measures.end(); i++)
		{
			for (std::vector<TrackNote>::iterator m = (*i).MeasureNotes.begin(); m != (*i).MeasureNotes.end(); m++)
			{
				/* We have to check for all gameplay conditions for this note. */
				if ((SongTime - m->GetTimeFinal()) * 1000 > score_keeper->getAccCutoff() && !m->WasNoteHit() && m->IsHold())
				{
					// remove hold notes that were never hit.
					MissNote((SongTime - m->GetTimeFinal()) * 1000, k, true);

					m = (*i).MeasureNotes.erase(m);

					if (score_keeper->getScore(ST_COMBO) > 10)
						MissSnd->Play();

					if (m == (*i).MeasureNotes.end())
						break;
				}

				else if ((SongTime - m->GetStartTime()) * 1000 > score_keeper->getAccCutoff() && (!m->WasNoteHit() && m->IsEnabled()))
				{
					// remove notes that were never hit.
					MissNote((SongTime - m->GetStartTime()) * 1000, k, false);

					if (score_keeper->getScore(ST_COMBO) > 10)
						MissSnd->Play();
					
					/* remove note from judgement */
					if (!m->IsHold())
						m = (*i).MeasureNotes.erase(m);
					else{
						m->Disable();
					}

					if (m == (*i).MeasureNotes.end())
						break;
				}

			}
		}
	}

}


#define CLAMP(var, min, max) (var) < (min) ? (min) : (var) > (max) ? (max) : (var)

void ScreenGameplay7K::ReleaseLane(unsigned int Lane)
{
	NoteVector &Measures = NotesByMeasure[Lane];

	for (NoteVector::iterator i = Measures.begin(); i != Measures.end(); i++)
	{
		for (std::vector<TrackNote>::iterator m = (*i).MeasureNotes.begin(); m != (*i).MeasureNotes.end(); m++)
		{
			if (m->WasNoteHit() && m->IsEnabled())
			{
				double tD = abs (m->GetTimeFinal() - SongTime) * 1000;

				if (tD < score_keeper->getAccCutoff()) /* Released in time */
				{
					HitNote(tD, Lane);

					HeldKey[m->GetTrack()] = false;
					(*i).MeasureNotes.erase(m);

				}else /* Released off time */
				{
					MissNote(tD, Lane, false);

					if (score_keeper->getScore(ST_COMBO) > 10)
						MissSnd->Play();

					m->Disable();
					HeldKey[m->GetTrack()] = false;
				}

				lastClosest[Lane] = std::min(tD, (double)lastClosest[Lane]);
				return;
			}
		}
	}
}

void ScreenGameplay7K::JudgeLane(unsigned int Lane)
{
	float MsDisplayMargin = (Configuration::GetSkinConfigf("HitErrorDisplayLimiter"));
	NoteVector &Measures = NotesByMeasure[Lane];

	if (!Music)
		return;

	lastClosest[Lane] = MsDisplayMargin;

	for (NoteVector::iterator i = Measures.begin(); i != Measures.end(); i++)
	{
		for (std::vector<TrackNote>::iterator m = (*i).MeasureNotes.begin(); m != (*i).MeasureNotes.end(); m++)
		{
			if (!m->IsEnabled())
				continue;

			double tD = abs (m->GetStartTime() - SongTime) * 1000;
			// std::cout << "\n time: " << m->GetStartTime() << " st: " << SongTime << " td: " << tD;

			lastClosest[Lane] = std::min(tD, (double)lastClosest[Lane]);

			if (lastClosest[Lane] == MsDisplayMargin)
			{
				lastClosest[Lane] = 0;
			}

			if (tD > score_keeper->getAccCutoff())
				continue;
			else
			{
				if (tD <= score_keeper->getAccMax())
				{
					HitNote(tD, Lane);

					if (m->IsHold())
					{
						m->Hit();
						HeldKey[m->GetTrack()] = true;
					}
				}
				else
				{
					MissNote(tD, Lane, m->IsHold());

					// missed feedback
					MissSnd->Play();

					if (m->IsHold()){
						m->Disable();
					}
				}
				

				/* remove note from judgement*/
				if (!m->IsHold())
				{
					(*i).MeasureNotes.erase(m);
				}

				return; // we judged a note in this lane, so we're done.
			}
		}
	}
}
