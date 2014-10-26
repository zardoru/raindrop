#include <cmath>

#include "GameGlobal.h"
#include "GameState.h"
#include "Logging.h"
#include "SongLoader.h"
#include "Screen.h"
#include "Audio.h"
#include "ImageLoader.h"
#include "GraphObject2D.h"
#include "Line.h"
#include "BitmapFont.h"
#include "GameWindow.h"
#include "ImageList.h"

#include "LuaManager.h"
#include "GraphObjectMan.h"

#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K.h"
#include "ScreenEvaluation7K.h"
#include "SongDatabase.h"

using namespace VSRG;

void ScreenGameplay7K::AssignMeasure(uint32 Measure)
{
	float Beat = 0;

	if (!Measure)
		return;

	for (uint32 i = 0; i < Measure; i++)
		Beat += CurrentDiff->Measures.at(Measure).MeasureLength;

	float Time = TimeAtBeat(CurrentDiff->Timing, CurrentDiff->Offset, Beat)
				+ StopTimeAtBeat(CurrentDiff->StopsTiming, Beat);

	// Disable all notes before the current measure.
	for (uint32 k = 0; k < Channels; k++)
	{
		for (std::vector<TrackNote>::iterator m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); m++)
		{
			if (m->GetStartTime() < Time)
			{
				m = NotesByChannel[k].erase(m);
				if (m == NotesByChannel[k].end()) break;
			}
		}
	}

	// Remove non-played objects
	for (std::vector<AutoplaySound>::iterator s = BGMEvents.begin(); s != BGMEvents.end(); s++)
	{
		if (s->Time <= Time)
		{
			s = BGMEvents.erase(s);
			if (s == BGMEvents.end()) break;
		}
	}

	SongTime = SongTimeReal = Time;

	if (Music)
		Music->SeekTime(Time);

	Active = true;
}

void ScreenGameplay7K::GearKeyEvent(uint32 Lane, bool KeyDown)
{
	Animations->GetEnv()->CallFunction("GearKeyEvent", 2);
	Animations->GetEnv()->PushArgument((int)Lane);
	Animations->GetEnv()->PushArgument(KeyDown);
	Animations->GetEnv()->RunFunction();

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

	if (KeyDown)
		JudgeLane(GearIndex, SongTime);
	else
		ReleaseLane(GearIndex, SongTime);
}

void ScreenGameplay7K::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	/*
	 In here we should use the input arrangements depending on
	 the amount of channels the current difficulty is using.
	 Also potentially pausing and quitting the screen.
	 Other than that most input can be safely ignored.
	*/

	/* Handle nested screens. */
	if (Next && Next->IsScreenRunning())
	{
		Next->HandleInput(key, code, isMouseInput);
		return;
	}

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
			{
				Active = true;
				Animations->DoEvent("OnActivateEvent");
			}
			break;
		case KT_FractionInc:
			SpeedMultiplierUser += 0.25;
			MultiplierChanged = true;
			break;
		case KT_FractionDec:
			SpeedMultiplierUser -= 0.25;
			MultiplierChanged = true;
			break;
		case KT_GoToEditMode:
			if (!Active)
			{
				Auto = !Auto;
				Animations->GetEnv()->SetGlobal("Auto", Auto);
			}
			break;
		}

		if (!Auto && BindingsManager::TranslateKey7K(key) != KT_Unknown)
			TranslateKey(BindingsManager::TranslateKey7K(key), true);

	}else
	{
		if (!Auto && BindingsManager::TranslateKey7K(key) != KT_Unknown)
			TranslateKey(BindingsManager::TranslateKey7K(key), false);
	}
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

void DoBMPEventList (GraphObject2D &Obj, std::vector<AutoplayBMP> &Events, ImageList &Images, double SongTime)
{
	for (std::vector<AutoplayBMP>::iterator b = Events.begin();
		b != Events.end();
		b++)
	{
		if (b->Time <= SongTime)
		{
			Image* Img = Images.GetFromIndex(b->BMP);
			if (Img != NULL)
				Obj.SetImage(Img, false);

			b = Events.erase(b);
			if (b == Events.end()) break;
		}
	}
}

void ScreenGameplay7K::RunAutoEvents()
{
	if (!stage_failed)
	{
		// Play BGM events.
		for (std::vector<AutoplaySound>::iterator s = BGMEvents.begin();
			s != BGMEvents.end();
			s++)
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
			}
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
	if (SongTime > CurrentDiff->Duration)
	{
		if (!SongFinished)
		{
			SongFinished = true;
			Animations->DoEvent("OnSongFinishedEvent", 1);
			SuccessTime = Clamp(Animations->GetEnv()->GetFunctionResultF(), 0.0f, 30.0f);
		}
	}

	// Reached the end!
	if (SuccessTime < 0 && SongFinished)
	{
		ScreenEvaluation7K *Eval = new ScreenEvaluation7K(this);
		Eval->Init(score_keeper);
		Next = Eval;
	}

	if (score_keeper->isStageFailed(lifebar_type) && !stage_failed)
	{
		// Run stage failed animation.
		stage_failed = true;
		Music->Stop();
		FailSnd->Play();

		for (std::map<int, SoundSample*>::iterator i = Keysounds.begin(); i != Keysounds.end(); i++)
		{
			if (i->second)
				i->second->Stop();
		}
			

		Animations->DoEvent("OnFailureEvent", 1);
		FailureTime = Clamp(Animations->GetEnv()->GetFunctionResultF(), 0.0f, 30.0f);
	}

	if (stage_failed)
	{
		MissTime = 10; // Infinite, for as long as it lasts.
		if (FailureTime <= 0)
			Running = false;
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
		SongTime += Delta;
	}

	// Update for the next delta.
	SongOldTime = SongTimeReal;

	// Run interpolation.
	if (Music && Music->IsPlaying() && !CurrentDiff->IsVirtual)
	{
		double SongDelta = Music->GetStreamedTime() - SongOldTime;
		SongTimeReal += SongDelta;

		if ( (SongDelta > 0.001 && abs(SongTime - SongTimeReal) * 1000 > ErrorTolerance) || !InterpolateTime ) // Significant delta with a x ms difference? We're pretty off..
			SongTime = SongTimeReal;
	}
}

bool ScreenGameplay7K::Run(double Delta)
{
	if (Next)
		return RunNested(Delta);

	if (!DoPlay)
		return false;

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
			CurrentVertical = IntegrateToTime(VSpeeds, SongTime);
		}
	}

	RecalculateEffects();
	RecalculateMatrix();

	UpdateScriptVariables();

	Animations->UpdateTargets(Delta);
	Render();

	return Running;
}
