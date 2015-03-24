#include <cmath>
#include <iostream>

#include "GameGlobal.h"
#include "GameState.h"
#include "Logging.h"
#include "SongLoader.h"
#include "Screen.h"
#include "Audio.h"
#include "ImageLoader.h"
#include "Sprite.h"
#include "Line.h"
#include "BitmapFont.h"
#include "GameWindow.h"
#include "ImageList.h"

#include "LuaManager.h"
#include "SceneEnvironment.h"

#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K.h"
#include "ScreenEvaluation7K.h"
#include "SongDatabase.h"

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
	return SectionValue(VSpeeds, SongTime);
}

float ScreenGameplay7K::GetCurrentVertical()
{
	return CurrentVertical;
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

	if (KeyDown)
		Keys[Lane].SetImage( GearLaneImageDown[Lane], false );
	else
		Keys[Lane].SetImage( GearLaneImage[Lane], false );
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

void ScreenGameplay7K::Activate()
{
	if (!Active)
	{
		Animations->DoEvent("OnActivateEvent");
	}

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
			Running = false;
			break;
		case KT_Enter:
			if (!Active)
				Activate();
			break;
		case KT_FractionInc:
			SpeedMultiplierUser += 0.25;
			MultiplierChanged = true;
			break;
		case KT_FractionDec:
			SpeedMultiplierUser -= 0.25;
			MultiplierChanged = true;
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

int DigitCount (float n)
{
	int digits = 0;

	while (n >= 1)
	{
		n /= 10;
		digits++;
	}

	return digits;
}

void DoBMPEventList (Sprite &Obj, std::vector<AutoplayBMP> &Events, ImageList &Images, double SongTime)
{
	for (auto b = Events.begin(); b != Events.end();)
	{
		if (b->Time <= SongTime)
		{
			Image* Img = Images.GetFromIndex(b->BMP);
			Obj.SetImage(Img, false);

			b = Events.erase(b);
			if (b == Events.end()) break;
			else continue;
		}

		b++;
	}
}

void ScreenGameplay7K::RunAutoEvents()
{
	if (!stage_failed)
	{
		// Play BGM events.
		for (auto s = BGMEvents.begin(); s != BGMEvents.end();)
		{
			if (s->Time <= SongTime)
			{
				if (Keysounds[s->Sound])
				{
					Keysounds[s->Sound]->SeekTime(SongTime - s->Time);
					Keysounds[s->Sound]->Play();
				}
				s = BGMEvents.erase(s);
				if (s == BGMEvents.end()) break;
				else continue;
			}

			s++;
		}
	}

	// Play BMP Base Events
	DoBMPEventList(Background, BMPEvents, BMPs, SongTime);

	// BMP Miss layer events
	DoBMPEventList(LayerMiss, BMPEventsMiss, BMPs, SongTime);

	// BMP Layer1 events
	DoBMPEventList(Layer1, BMPEventsLayer, BMPs, SongTime);

	// BMP Layer 2 events.
	DoBMPEventList(Layer2, BMPEventsLayer2, BMPs, SongTime);
}

void ScreenGameplay7K::CheckShouldEndScreen()
{
	// Run failure first; make sure it has priority over checking whether it's a pass or not.
	if (score_keeper->isStageFailed(lifebar_type) && !stage_failed && !NoFail)
	{
		// We make sure we don't trigger this twice.
		stage_failed = true;
		score_keeper->failStage();
		FailSnd->Play();

		// We stop all audio..
		Music->Stop();
		for (std::map<int, SoundSample*>::iterator i = Keysounds.begin(); i != Keysounds.end(); i++)
		{
			if (i->second)
				i->second->Stop();
		}

		// Run stage failed animation.
		Animations->DoEvent("OnFailureEvent", 1);
		FailureTime = Clamp(Animations->GetEnv()->GetFunctionResultF(), 0.0f, 30.0f);
	}

	// Okay then, so it's a pass?
	if (SongTime > CurrentDiff->Duration && !stage_failed)
	{
		double curBPS = SectionValue(BPS, SongTime);
		double cutoffspb = 1 / curBPS;
		double cutoffTime;

		if (score_keeper->usesO2()) // beat-based judgements
			cutoffTime = cutoffspb * score_keeper->getMissCutoff();
		else // time-based judgments
			cutoffTime = score_keeper->getMissCutoff() / 1000.0;

		// we need to make sure we trigger this AFTER all notes could've possibly been judged
		// note to self: songtime will always be positive since duration is always positive.
		// cutofftime is unlikely to ever be negative.
		if (SongTime - CurrentDiff->Duration > cutoffTime) 
		{
			if (!SongFinished)
			{
				SongFinished = true; // Reached the end!
				Animations->DoEvent("OnSongFinishedEvent", 1);
				SuccessTime = Clamp(Animations->GetEnv()->GetFunctionResultF(), 0.0f, 30.0f);
			}
		}
	}

	// Okay then, the song's done, and the success animation is done too. Time to evaluate.
	if (SuccessTime < 0 && SongFinished)
	{
		ScreenEvaluation7K *Eval = new ScreenEvaluation7K(this);
		Eval->Init(score_keeper);
		Next = Eval;
	}

	if (stage_failed)
	{
		MissTime = 10; // Infinite, for as long as it lasts.
		if (FailureTime <= 0){ // go to evaluation screen, or back to song select depending on the skin

			if (Configuration::GetSkinConfigf("GoToSongSelectOnFailure") == 0)
			{
				ScreenEvaluation7K *Eval = new ScreenEvaluation7K(this);
				Eval->Init(score_keeper);
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
	CurrentBeat = IntegrateToTime(BPS, SongTime);
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
			
			CurrentVertical = IntegrateToTime(VSpeeds, SongTime);

			RunAutoEvents();
			RunMeasures();
			CheckShouldEndScreen();
		}else
		{
			SongTime = -(WaitingTime - GameTime);
			CurrentBeat = IntegrateToTime(BPS, SongTime);
			CurrentVertical = IntegrateToTime(VSpeeds, SongTime);
		}
	}
	else
	{
		CurrentVertical = IntegrateToTime(VSpeeds, -WaitingTime);
		CurrentBeat = IntegrateToTime(BPS, SongTime);
	}

	RecalculateEffects();
	RecalculateMatrix();

	UpdateScriptVariables();

	Animations->UpdateTargets(Delta);
	Render();

	if (Delta > 0.1)
		Log::Logf("ScreenGameplay7K: Delay@[ST%.03f/RST:%.03f] = %f\n", GetScreenTime(), SongTime, Delta);

	return Running;
}
