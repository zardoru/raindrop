#include <cmath>
#include <iostream>

#include "GameGlobal.h"
#include "Logging.h"
#include "Screen.h"
#include "Audio.h"


#include "LuaManager.h"
#include "SceneEnvironment.h"

#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K.h"
#include "ScreenEvaluation7K.h"
#include "Noteskin.h"

using namespace VSRG;

bool ScreenGameplay7K::IsAutoEnabled()
{
	return Auto;
}

bool ScreenGameplay7K::IsFailEnabled()
{
	return NoFail;
}

float ScreenGameplay7K::GetCurrentBeat()
{
	return CurrentBeat;
}

float ScreenGameplay7K::GetUserMultiplier() const
{
	return SpeedMultiplierUser;
}

float ScreenGameplay7K::GetCurrentVerticalSpeed()
{
	return SectionValue(VSpeeds, WarpedSongTime);
}

float ScreenGameplay7K::GetCurrentVertical()
{
	return CurrentVertical;
}

double ScreenGameplay7K::GetWarpedSongTime()
{
	// We can't use lower_bound; Every warp changes the time we actually compare.
	auto T = SongTime;
	for (auto k = Warps.begin(); k != Warps.end(); ++k)
	{
		if (k->Time <= T)
			T += k->Value;
	}

	return T;
}

double ScreenGameplay7K::GetSongTime()
{
	double usedTime = -1;

	if (UsedTimingType == TT_BEATS)
		usedTime = CurrentBeat;
	else if (UsedTimingType == TT_TIME)
		usedTime = SongTime;

	assert(usedTime != -1);
	return usedTime;
}

void ScreenGameplay7K::SetUserMultiplier(float Multip)
{
	if (SongTime <= 0 || !Active)
		SpeedMultiplierUser = Multip;
}


void ScreenGameplay7K::GearKeyEvent(uint32 Lane, bool KeyDown)
{
	if (Animations->GetEnv()->CallFunction("GearKeyEvent", 2))
	{
		Animations->GetEnv()->PushArgument((int)Lane);
		Animations->GetEnv()->PushArgument(KeyDown);
		Animations->GetEnv()->RunFunction();
	}
}

void ScreenGameplay7K::TranslateKey(KeyType K, bool KeyDown)
{
	int Index = K - KT_Key1; /* Bound key */
	int GearIndex = GearBindings[Index]; /* Binding this key to a lane */

	if (Index >= MAX_CHANNELS || Index < 0)
		return;

	if (GearIndex >= MAX_CHANNELS || GearIndex < 0)
		return;

	if (KeyDown){
		JudgeLane(GearIndex, GetSongTime());
		GearIsPressed[GearIndex] = true;
	}else{
		ReleaseLane(GearIndex, GetSongTime());
		GearIsPressed[GearIndex] = false;
	}
}

bool ScreenGameplay7K::IsUpscrolling()
{
	return SpeedMultiplierUser < 0 || Upscroll;
}

void ScreenGameplay7K::Activate()
{
	if (!Active)
		Animations->DoEvent("OnActivateEvent");

	Active = true;
}

bool ScreenGameplay7K::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	/*
	 In here we should use the input arrangements depending on
	 the amount of channels the current difficulty is using.
	 Also potentially pausing and quitting the screen.
	 Other than that most input can be safely ignored.
	*/

	/* Handle nested screens. */
	if (Screen::HandleInput(key, code, isMouseInput))
		return true;

	Animations->HandleInput(key, code, isMouseInput);

	if (code == KE_Press)
	{
		switch (BindingsManager::TranslateKey(key))
		{
		case KT_Escape:
			if (SongFinished)
				SuccessTime = -1;
			else
				Running = false;
			break;
		case KT_Enter:
			if (!Active)
				Activate();
			break;
		case KT_Right:
			SpeedMultiplierUser += 0.25;
			MultiplierChanged = true;
			break;
		case KT_Left:
			SpeedMultiplierUser -= 0.25;
			MultiplierChanged = true;
			break;
		default:
			break;
		}

		if (!Auto && BindingsManager::TranslateKey7K(key) != KT_Unknown)
			TranslateKey(BindingsManager::TranslateKey7K(key), true);

	}else
	{
		if (!Auto && BindingsManager::TranslateKey7K(key) != KT_Unknown)
			TranslateKey(BindingsManager::TranslateKey7K(key), false);
	}

	return true;
}

void ScreenGameplay7K::RunAutoEvents()
{
	if (!stage_failed)
	{
		// Play BGM events.
		auto i = BGMEvents.begin();
		auto s = std::upper_bound(BGMEvents.begin(), BGMEvents.end(), SongTime);
		while (i != s)
		{
			if (Keysounds[i->Sound]) Keysounds[i->Sound]->Play();

			i = BGMEvents.erase(i);
			s = std::lower_bound(BGMEvents.begin(), BGMEvents.end(), SongTime);
		}
	}

	BGA->SetAnimationTime(SongTime);
}

void ScreenGameplay7K::CheckShouldEndScreen()
{
	// Run failure first; make sure it has priority over checking whether it's a pass or not.
	if (ScoreKeeper->isStageFailed(lifebar_type) && !stage_failed && !NoFail)
	{
		// We make sure we don't trigger this twice.
stageFailed:
		stage_failed = true;
		ScoreKeeper->failStage();
		FailSnd->Play();

		// We stop all audio..
		Music->Stop();
		for (auto i = Keysounds.begin(); i != Keysounds.end(); ++i)
		{
			if (i->second)
				i->second->Stop();
		}

		// Run stage failed animation.
		Animations->DoEvent("OnFailureEvent", 1);
		FailureTime = Clamp(Animations->GetEnv()->GetFunctionResultF(), 0.0f, 30.0f);
	}

	// Okay then, so it's a pass?
	if (WarpedSongTime > CurrentDiff->Duration && !stage_failed)
	{
		double curBPS = SectionValue(BPS, WarpedSongTime);
		double cutoffspb = 1 / curBPS;
		double cutoffTime;

		if (ScoreKeeper->usesO2()) // beat-based judgements
			cutoffTime = cutoffspb * ScoreKeeper->getMissCutoff();
		else // time-based judgments
			cutoffTime = ScoreKeeper->getMissCutoff() / 1000.0;

		// we need to make sure we trigger this AFTER all notes could've possibly been judged
		// note to self: songtime will always be positive since duration is always positive.
		// cutofftime is unlikely to ever be negative.
		if (WarpedSongTime - CurrentDiff->Duration > cutoffTime)
		{
			if (!SongFinished)
			{
				if (ScoreKeeper->isStageFailed(lifebar_type) && !NoFail)
					goto stageFailed; // No, don't trigger SongFinished. It wasn't a pass.

				SongFinished = true; // Reached the end!
				Animations->DoEvent("OnSongFinishedEvent", 1);
				SuccessTime = Clamp(Animations->GetEnv()->GetFunctionResultF(), 3.0f, 30.0f);
			}
		}
	}

	// Okay then, the song's done, and the success animation is done too. Time to evaluate.
	if (SuccessTime < 0 && SongFinished)
	{
		ScreenEvaluation7K *Eval = new ScreenEvaluation7K(this);
		Eval->Init(ScoreKeeper.get());
		Next = Eval;
	}

	if (stage_failed)
	{
		MissTime = 10; // Infinite, for as long as it lasts.
		if (FailureTime <= 0){ // go to evaluation screen, or back to song select depending on the skin

			if (Configuration::GetSkinConfigf("GoToSongSelectOnFailure") == 0)
			{
				auto Eval = new ScreenEvaluation7K(this);
				Eval->Init(ScoreKeeper.get());
				Next = Eval;
			}
			else
				Running = false;
		}
	}
}

void ScreenGameplay7K::UpdateSongTime(float Delta)
{
	// Check if we should play the music..
	if (SongOldTime == -1)
	{
		if (Music)
			Music->Play();
		if (!StartMeasure)
		{
			SongOldTime = 0;
			SongTimeReal = 0;
			SongTime = 0;
		}
	}else
	{
		/* Update music. */
		SongTime += Delta * Speed;
	}

	// Update for the next delta.
	SongOldTime = SongTimeReal;

	// Run interpolation, if enabled.
	if (Music && Music->IsPlaying() && !CurrentDiff->IsVirtual)
	{
		double SongDelta = Music->GetStreamedTime() - SongOldTime;
		SongTimeReal += SongDelta;

		if ( (SongDelta > 0.001 && abs(SongTime - SongTimeReal) * 1000 > ErrorTolerance) || !InterpolateTime ) // Significant delta with a x ms difference? We're pretty off..
			SongTime = SongTimeReal;
	}

	// Update current beat
	WarpedSongTime = GetWarpedSongTime();
	CurrentBeat = IntegrateToTime(BPS, WarpedSongTime);
}


bool ScreenGameplay7K::Run(double Delta)
{
	if (Next)
		return RunNested(Delta);

	if (!DoPlay)
		return false;

	if (ForceActivation)
	{
		Activate();
		ForceActivation = false;
	}

	if (Active)
	{
		GameTime += Delta;
		MissTime -= Delta;
		FailureTime -= Delta;
		SuccessTime -= Delta;

		if (GameTime >= WaitingTime)
		{
			UpdateSongTime(Delta);
			
			CurrentVertical = IntegrateToTime(VSpeeds, WarpedSongTime);

			RunAutoEvents();
			RunMeasures();
			CheckShouldEndScreen();
		}else
		{
			SongTime = -(WaitingTime - GameTime);
			CurrentBeat = IntegrateToTime(BPS, SongTime);
			WarpedSongTime = SongTime;
			CurrentVertical = IntegrateToTime(VSpeeds, SongTime);
		}
	}
	else
	{
		CurrentVertical = IntegrateToTime(VSpeeds, -WaitingTime);
		CurrentBeat = IntegrateToTime(BPS, SongTime);
		WarpedSongTime = -WaitingTime;
	}

	Noteskin::Update(SongTime, CurrentBeat);
	RecalculateEffects();

	UpdateScriptVariables();

	Animations->UpdateTargets(Delta);
	Render();

	if (Delta > 0.1)
		Log::Logf("ScreenGameplay7K: Delay@[ST%.03f/RST:%.03f] = %f\n", GetScreenTime(), SongTime, Delta);

	return Running;
}
