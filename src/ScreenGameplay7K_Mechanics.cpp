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
#include "ScreenGameplay7K_Mechanics.h"

#include <iostream>
#include <iomanip>
#include <cmath>

using namespace VSRG;

void ScreenGameplay7K::RecalculateMatrix()
{
	PositionMatrix = glm::translate(Mat4(), glm::vec3(0, JudgmentLinePos + CurrentVertical * SpeedMultiplier, 0));
	PositionMatrixJudgment = glm::translate(Mat4(), glm::vec3(0, JudgmentLinePos, 0));

	for (uint8 i = 0; i < CurrentDiff->Channels; i++)
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


void ScreenGameplay7K::HitNote (double TimeOff, uint32 Lane, bool IsHold, bool IsHoldRelease)
{
	int Judgment = score_keeper->hitNote(TimeOff);

	UpdateScriptScoreVariables();

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

	// per-theme seconds showing miss bga
	MissTime = Configuration::GetSkinConfigf("OnMissBGATime");

	UpdateScriptScoreVariables();

	Animations->GetEnv()->CallFunction("MissEvent", 3);
	Animations->GetEnv()->PushArgument(TimeOff);
	Animations->GetEnv()->PushArgument((int)Lane + 1);
	Animations->GetEnv()->PushArgument(IsHold);
	Animations->GetEnv()->RunFunction();
}


void ScreenGameplay7K::PlayLaneKeysound(uint32 Lane)
{
	if (Keysounds[PlaySounds[Lane]] && PlayReactiveSounds)
		Keysounds[PlaySounds[Lane]]->Play();
}

void ScreenGameplay7K::PlayKeysound(uint32 Index)
{
	if (Keysounds[Index])
		Keysounds[Index]->Play();
}

void ScreenGameplay7K::SetLaneHoldState(uint32 Lane, bool NewState)
{
	HeldKey[Lane] = NewState;
}

// true if holding down key
bool ScreenGameplay7K::GetGearLaneState(uint32 Lane)
{
	return GearIsPressed[Lane] != 0;
}

void ScreenGameplay7K::RunMeasures()
{
	double timeClosest[VSRG::MAX_CHANNELS];

	for (int i = 0; i < VSRG::MAX_CHANNELS; i++)
		timeClosest[i] = CurrentDiff->Duration;

	double usedTime;

	if (UsedTimingType == TT_TIME)
		usedTime = SongTime;
	else if (UsedTimingType == TT_BEATS)
		usedTime = CurrentBeat;

	for (uint16 k = 0; k < CurrentDiff->Channels; k++)
	{
		for (std::vector<TrackNote>::iterator m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); m++)	{

			// Keysound update to closest note.
			if (CurrentDiff->IsVirtual)	{
				if (m->IsEnabled())
				{
					if ((abs(usedTime - m->GetTimeFinal()) < timeClosest[k]))
					{
						PlaySounds[k] = m->GetSound();
						timeClosest[k] = abs(usedTime - m->GetTimeFinal());
					}else
						break; // In other words, we're getting further away.
				}
			}


			if (Auto) {
				double TimeThreshold = usedTime + 0.008; // latest time a note can activate.
				if ( m->GetStartTime() <= TimeThreshold)
				{
					if (m->IsEnabled()) {
						if (m->IsHold())
						{
							if (m->WasNoteHit())
							{
								if (m->GetTimeFinal() < TimeThreshold){
									double hit_time = clamp_to_interval(usedTime, m->GetTimeFinal(), 0.008);
									// We use clamp_to_interval for those pesky outliers.
									ReleaseLane(k, hit_time);
									// ReleaseLane(k, m->GetTimeFinal());
								}
							}else{
								double hit_time = clamp_to_interval(usedTime, m->GetStartTime(), 0.008);
								JudgeLane(k, hit_time);
								// JudgeLane(k, m->GetStartTime());
							}
						}else
						{
							double hit_time = clamp_to_interval(usedTime, m->GetStartTime(), 0.008);
							JudgeLane(k, hit_time);
							ReleaseLane(k, hit_time);
							// JudgeLane(k, m->GetStartTime());
							// ReleaseLane(k, m->GetTimeFinal());
						}
					}
				}
			}


			if(stage_failed) continue; // don't check for judgments after stage has failed.

			if (MechanicsSet->OnUpdate(usedTime, &(*m), k))
				break;
		} // end for notes
	} // end for channels
}

void ScreenGameplay7K::ReleaseLane(uint32 Lane, float Time)
{
	GearKeyEvent(Lane, false);

	if(stage_failed) return; // don't judge any more after stage is failed.

	for (std::vector<TrackNote>::iterator m = NotesByChannel[Lane].begin(); m != NotesByChannel[Lane].end(); m++)
	{
		if (MechanicsSet->OnReleaseLane(Time, &(*m), Lane)) // Are we done judging..?
			break;
	}
}

void ScreenGameplay7K::JudgeLane(uint32 Lane, float Time)
{
	GearKeyEvent(Lane, true);

	if ( (!Music && !CurrentDiff->IsVirtual) || !Active || stage_failed)
		return;

	lastClosest[Lane] = MsDisplayMargin;

	for (std::vector<TrackNote>::iterator m = NotesByChannel[Lane].begin(); m != NotesByChannel[Lane].end(); m++)
	{
		double dev = (SongTime - m->GetStartTime()) * 1000;
		double tD = abs(dev);

		lastClosest[Lane] = min(tD, (double)lastClosest[Lane]);

		if (lastClosest[Lane] >= MsDisplayMargin)
			lastClosest[Lane] = 0;

		if (MechanicsSet->OnPressLane(Time, &(*m), Lane))
			return; // we judged a note in this lane, so we're done.

	}
}
