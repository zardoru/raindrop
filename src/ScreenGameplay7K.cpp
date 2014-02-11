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

BitmapFont * GFont = NULL;
SoundSample *MissSnd = NULL;
int lastPressed = 0;
int lastMsOff[MAX_CHANNELS];
int lastClosest[MAX_CHANNELS];

/* Time before actually starting everything. */
#define WAITING_TIME 1.5

int holds_missed = 0;
int holds_hit = 0;

ScreenGameplay7K::ScreenGameplay7K()
{
	SpeedMultiplier = 0;
	SongOldTime = -1;
	Music = NULL;
	deltaPos = 0;

	waveEffectEnabled = false;
	waveEffect = 0;

	SpeedMultiplierUser = 4;

	CurrentVertical = 0;
	SongTime = 0;

	AudioCompensation = (Configuration::GetConfigf("AudioCompensation") != 0);
	TimeCompensation = 0;

	if (!GFont)
	{	
		GFont = new BitmapFont();
		GFont->LoadSkinFontImage("font.tga", Vec2(6, 15), Vec2(8, 16), Vec2(6, 15), 0);
	}

	Active = false;
}

void ScreenGameplay7K::Cleanup()
{
	if (Music)
	{
		MixerRemoveStream(Music);
		Music->Stop();
	}

	delete Animations;
	delete score_keeper;
}

void ScreenGameplay7K::Init(Song7K* S, int DifficultyIndex, bool UseUpscroll)
{
	MySong = S;
	CurrentDiff = S->Difficulties[DifficultyIndex];
	Upscroll = UseUpscroll;

	holds_hit = 0;
	holds_missed = 0;

	Animations = new GraphObjectMan();

	score_keeper = new ScoreKeeper7K();
	score_keeper->setMaxNotes(CurrentDiff->TotalScoringObjects);
}

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

#define CLAMP(var, min, max) (var) < (min) ? (min) : (var) > (max) ? (max) : (var)

void ScreenGameplay7K::HitNote (double TimeOff, uint32 Lane)
{
	score_keeper->hitNote(TimeOff);

	Animations->GetEnv()->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	Animations->GetEnv()->CallFunction("HitEvent", 2);
	Animations->GetEnv()->PushArgument(TimeOff);
	Animations->GetEnv()->PushArgument((int)Lane + 1);
	Animations->GetEnv()->RunFunction();

}

void ScreenGameplay7K::MissNote (double TimeOff, uint32 Lane)
{

	Animations->GetEnv()->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	Animations->GetEnv()->CallFunction("MissEvent", 2);
	Animations->GetEnv()->PushArgument(TimeOff);
	Animations->GetEnv()->PushArgument((int)Lane + 1);
	Animations->GetEnv()->RunFunction();

}

void ScreenGameplay7K::RunMeasures()
{
	typedef std::vector<SongInternal::Measure<TrackNote> > NoteVector;

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
					MissNote((SongTime - m->GetTimeFinal()) * 1000, k);
					score_keeper->missNote(true);
					holds_missed += 1;
					m = (*i).MeasureNotes.erase(m);

					if (score_keeper->getScore(ST_COMBO) > 10)
						MissSnd->Play();

					if (m == (*i).MeasureNotes.end())
						break;
				}

				else if ((SongTime - m->GetStartTime()) * 1000 > score_keeper->getAccCutoff() && (!m->WasNoteHit() && m->IsEnabled()))
				{
					// remove notes that were never hit.
					MissNote((SongTime - m->GetStartTime()) * 1000, k);
					score_keeper->missNote(false);

					if (score_keeper->getScore(ST_COMBO) > 10)
						MissSnd->Play();
					
					/* remove note from judgement */
					if (!m->IsHold())
						m = (*i).MeasureNotes.erase(m);
					else{
						holds_missed += 1;
						m->Disable();
					}

					if (m == (*i).MeasureNotes.end())
						break;
				}

			}
		}
	}

}

void ScreenGameplay7K::ReleaseLane(unsigned int Lane)
{
	typedef std::vector<SongInternal::Measure<TrackNote> > NoteVector;
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
					holds_hit += 1;

					(*i).MeasureNotes.erase(m);

					lastClosest[Lane] = std::min(tD, (double)lastClosest[Lane]);

					return;
				}else /* Released off time */
				{
					MissNote(tD, Lane);
					score_keeper->missNote(false);
					holds_missed += 1;

					if (score_keeper->getScore(ST_COMBO) > 10)
						MissSnd->Play();

					m->Disable();

					lastClosest[Lane] = std::min(tD, (double)lastClosest[Lane]);

					return;
				}
			}
		}
	}

	HeldKey[Lane] = NULL;
}

void ScreenGameplay7K::JudgeLane(unsigned int Lane)
{
	typedef std::vector<SongInternal::Measure<TrackNote> > NoteVector;
	NoteVector &Measures = NotesByMeasure[Lane];

	lastPressed = Lane;

	if (!Music)
		return;

	lastClosest[Lane] = 9999;

	for (NoteVector::iterator i = Measures.begin(); i != Measures.end(); i++)
	{
		for (std::vector<TrackNote>::iterator m = (*i).MeasureNotes.begin(); m != (*i).MeasureNotes.end(); m++)
		{
			if (!m->IsEnabled())
				continue;

			double tD = abs (m->GetStartTime() - SongTime) * 1000;
			// std::cout << "\n time: " << m->GetStartTime() << " st: " << SongTime << " td: " << tD;

			lastClosest[Lane] = std::min(tD, (double)lastClosest[Lane]);

			if (tD > score_keeper->getAccCutoff())
				continue;
			else
			{
				if (tD <= score_keeper->getAccMax())
				{
					HitNote(tD, Lane);

					if (m->IsHold())
					{
						holds_hit += 1;
						m->Hit();
						HeldKey[m->GetTrack()] = true;
					}
				}
				else
				{
					MissNote(tD, Lane);
					score_keeper->missNote(m->IsHold());

					// missed feedback
					MissSnd->Play();

					if (m->IsHold()){
						holds_missed += 1;
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

void ScreenGameplay7K::RecalculateMatrix()
{
	PositionMatrix = glm::translate(Mat4(), glm::vec3(0, BasePos + CurrentVertical * SpeedMultiplier + deltaPos, 0));
}

void ScreenGameplay7K::LoadThreadInitialization()
{
	if (!MissSnd)
	{
		MissSnd = new SoundSample();
		MissSnd->Open((FileManager::GetSkinPrefix() + "miss.ogg").c_str());
		MixerAddSample(MissSnd);
	}

	/* Can I just use a vector<char**> and use vector.data()? */
	char* SkinFiles [] =
	{
		"key1.png",
		"key2.png",
		"key3.png",
		"key4.png",
		"key5.png",
		"key6.png",
		"key7.png",
		"key8.png",
		"key1d.png",
		"key2d.png",
		"key3d.png",
		"key4d.png",
		"key5d.png",
		"key6d.png",
		"key7d.png",
		"key8d.png",
		"note.png",
		"judgeline.png"
	};

	ImageLoader::LoadFromManifest(SkinFiles, 3, FileManager::GetSkinPrefix());
	
	char* OtherFiles [] =
	{
		(char*)MySong->BackgroundDir.c_str()
	};

	ImageLoader::LoadFromManifest(OtherFiles, 1);
	/* TODO: Add playfield background */

	if (!Music)
	{
		Music = new AudioStream();
		Music->Open(MySong->SongFilename.c_str());
		MixerAddStream(Music);
	}

	if (AudioCompensation)
		TimeCompensation = MixerGetLatency();

	MySong->Process(TimeCompensation + Configuration::GetConfigf("Offset7K"));

	Channels = CurrentDiff->Channels;
	VSpeeds = CurrentDiff->VerticalSpeeds;

	/* Set up notes. */
	for (uint32 k = 0; k < Channels; k++)
	{
		/* Copy measures. (Eek!) */
		for (std::vector<SongInternal::Measure<TrackNote> >::iterator i = CurrentDiff->Measures[k].begin(); 
			i != CurrentDiff->Measures[k].end();
			i++)
		{
			NotesByMeasure[k].push_back(*i);
		}
	}

	NoteHeight = Configuration::GetSkinConfigf("NoteHeight7K");

	if (!NoteHeight)
		NoteHeight = 10;

	char str[256];
	char nstr[256];

	sprintf(nstr, "Channels%d", CurrentDiff->Channels);

	sprintf(str, "GearHeight");
	GearHeightFinal = Configuration::GetSkinConfigf(str, nstr);

	/* Initial object distance */
	if (!Upscroll)
		JudgementLinePos = float(ScreenHeight) - GearHeightFinal;
	else
		JudgementLinePos = GearHeightFinal;

	BasePos = JudgementLinePos + (Upscroll ? NoteHeight/2 : -NoteHeight/2);
	CurrentVertical -= VSpeeds.at(0).Value * (WAITING_TIME);

	RecalculateMatrix();
	MultiplierChanged = true;
}

void ScreenGameplay7K::SetupScriptConstants()
{
	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("Upscroll", Upscroll);
	L->SetGlobal("Channels", Channels);
	L->SetGlobal("JudgementLineY", JudgementLinePos);
	L->SetGlobal("AccuracyHitMS", score_keeper->getAccMax());
}

void ScreenGameplay7K::SetupGear()
{
	using namespace Configuration;
	char str[256];
	char cstr[256];
	char nstr[256];

	sprintf(nstr, "Channels%d", CurrentDiff->Channels);

	for (int i = 0; i < CurrentDiff->Channels; i++)
	{
		sprintf(cstr, "Key%d", i+1);
		
		/* If it says that the nth lane uses the kth key then we'll bind that! */
		sprintf(str, "key%d.png", (int)GetSkinConfigf(cstr, nstr));
		GearLaneImage[i] = ImageLoader::LoadSkin(str);

		sprintf(str, "key%dd.png", (int)GetSkinConfigf(cstr, nstr));
		GearLaneImageDown[i] = ImageLoader::LoadSkin(str);

		sprintf(str, "Key%dX", i+1);
		LanePositions[i] = Configuration::GetSkinConfigf(str, nstr);

		sprintf(str, "Key%dWidth", i+1);
		LaneWidth[i] = Configuration::GetSkinConfigf(str, nstr);

		Keys[i].SetImage ( GearLaneImage[i] );
		Keys[i].SetSize( LaneWidth[i], GearHeightFinal );
		Keys[i].Centered = true;

		Keys[i].SetPosition( LanePositions[i], JudgementLinePos + (Upscroll? -1:1) * GearHeightFinal/2 );

		if (Upscroll)
			Keys[i].SetRotation(180);

		Keys[i].SetZ(15);

		NoteMatrix[i] = glm::translate(Mat4(), glm::vec3(LanePositions[i], 0, 14)) * glm::scale(Mat4(), glm::vec3(LaneWidth[i], NoteHeight, 1));
	}
}

void ScreenGameplay7K::MainThreadInitialization()
{
	SetupGear();

	char nstr[256];
	sprintf(nstr, "Channels%d", CurrentDiff->Channels);

	for (int i = 0; i < CurrentDiff->Channels; i++)
	{
		std::stringstream ss;
		char cstr[256];
		
		/* Assign per-lane bindings. */
		sprintf(cstr, "Key%dBinding", i+1);

		int Binding = Configuration::GetSkinConfigf(cstr, nstr);
		GearBindings[Binding - 1] = i;

		/* Note image */
		sprintf(cstr, "Key%dImage", i+1);

		std::string Filename = Configuration::GetSkinConfigs(cstr, nstr);
		NoteImages[i] = ImageLoader::LoadSkin(Filename);

		/* Hold image */
		sprintf(cstr, "Key%dHoldImage", i+1);

		Filename = Configuration::GetSkinConfigs(cstr, nstr);
		NoteImagesHold[i] = ImageLoader::LoadSkin(Filename);

		lastClosest[i] = 0;

		HeldKey[i] = NULL;
	}

	NoteImage = ImageLoader::LoadSkin("note.png");

	Background.SetImage(ImageLoader::Load(MySong->BackgroundDir));
	Background.SetZ(0);
	Background.AffectedByLightning = true;

	if (Background.GetImage())
	{
		float SizeRatio = 768 / Background.GetHeight();
		Background.SetScale(SizeRatio);
		Background.Centered = true;
		Background.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	}

	WindowFrame.SetLightMultiplier(0.45f);

	SetupScriptConstants();
	Animations->Initialize( FileManager::GetSkinPrefix() + "screengameplay7k.lua" );

	Running = true;
}

void ScreenGameplay7K::TranslateKey(KeyType K, bool KeyDown)
{
	int Index = K - KT_Key1; /* Bound key */
	int GearIndex = GearBindings[Index]; /* Binding this key to a lane */

	if (Index >= MAX_CHANNELS || Index < 0)
		return;

	if (GearIndex >= MAX_CHANNELS || GearIndex < 0)
		return;

	Animations->GetEnv()->CallFunction("GearKeyEvent", 2);
	Animations->GetEnv()->PushArgument(GearIndex);
	Animations->GetEnv()->PushArgument(KeyDown);
	Animations->GetEnv()->RunFunction();

	if (KeyDown)
	{
		JudgeLane(GearIndex);
		Keys[GearIndex].SetImage( GearLaneImageDown[GearIndex], false );
	}
	else
	{
		ReleaseLane(GearIndex);
		Keys[GearIndex].SetImage( GearLaneImage[GearIndex], false );
	}
}

void ScreenGameplay7K::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	/* 
	 In here we should use the input arrangements depending on 
	 the amount of channels the current difficulty is using.
	 Also potentially pausing and quitting the screen.
	 Other than that most input can be safely ignored.
	*/

	Animations->HandleInput(key, code, isMouseInput);

	if (code == KE_Press)
	{
		switch (BindingsManager::TranslateKey(key))
		{
		case KT_Escape:
			Running = false;
			break;
		case KT_Enter:
			Active = true;
			break;
		case KT_FractionInc:
			SpeedMultiplierUser += 0.25;
			MultiplierChanged = true;
			break;
		case KT_FractionDec:
			SpeedMultiplierUser -= 0.25;
			MultiplierChanged = true;
			break;
		case KT_Left:
			deltaPos -= 10;
			break;
		case KT_Right:
			deltaPos += 10;
			break;
		case KT_GoToEditMode:
			waveEffectEnabled = !waveEffectEnabled;
			break;
		}

		if (BindingsManager::TranslateKey7K(key) != KT_Unknown)
			TranslateKey(BindingsManager::TranslateKey7K(key), true);

	}else
	{
		if (BindingsManager::TranslateKey7K(key) != KT_Unknown)
			TranslateKey(BindingsManager::TranslateKey7K(key), false);
	}
}

void ScreenGameplay7K::UpdateScriptVariables()
{
	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("Combo", score_keeper->getScore(ST_COMBO));
	L->SetGlobal("MaxCombo", score_keeper->getScore(ST_MAX_COMBO));
	L->SetGlobal("SpeedMultiplier", SpeedMultiplier);
	L->SetGlobal("SpeedMultiplierUser", SpeedMultiplierUser);
	L->SetGlobal("waveEffectEnabled", waveEffectEnabled);
	L->SetGlobal("Accuracy", score_keeper->getPercentScore(PST_ACC));
	L->SetGlobal("EXScore", score_keeper->getPercentScore(PST_EX));
	L->SetGlobal("Active", Active);
	L->SetGlobal("Beat", BeatAtTime(CurrentDiff->BPS, SongTime, CurrentDiff->Offset + TimeCompensation));
}

bool ScreenGameplay7K::Run(double Delta)
{
	float SongDelta;

	if (Active)
	{
		ScreenTime += Delta;

		if (ScreenTime > WAITING_TIME)
		{

			if (!Music)
				return false; // Quit inmediately. There's no point.

			if (SongOldTime == -1)
			{
				Music->Play();
				SongOldTime = 0;
				SongTimeReal = 0;
				SongTime = 0;
			}else
			{
				/* Update music. */
				SongTime += Delta;
			}

			SongDelta = Music->GetStreamedTime() - SongOldTime;
			SongTimeReal += SongDelta;

			if (SongDelta > 0.00001 && abs(SongTime - SongTimeReal) > 0.005) // Significant delta with a 5 ms difference? We're pretty off!
				SongTime = SongTimeReal;

			CurrentVertical = VerticalAtTime(VSpeeds, SongTime);
			RunMeasures();

			SongOldTime = SongTimeReal;
		}else
		{
			SongTime = -(WAITING_TIME - ScreenTime);
			SongDelta = 0;
			CurrentVertical = VerticalAtTime(VSpeeds, SongTime);
		}
	}

	RecalculateEffects();
	RecalculateMatrix();
		
	UpdateScriptVariables();

	Animations->UpdateTargets(Delta);

	Background.Render();

	Animations->DrawUntilLayer(13);

	DrawMeasures();

	for (int32 i = 0; i < CurrentDiff->Channels; i++)
		Keys[i].Render();

	std::stringstream ss;

	ss << "\nscore: " << score_keeper->getScore(ST_SCORE);
	ss << "\nnotes hit: " << score_keeper->getScore(ST_NOTES_HIT);
	ss << "\nEX score: " << score_keeper->getScore(ST_EX);
	ss << "\nMult/Speed: " << std::setprecision(2) << std::setiosflags(std::ios::fixed) << SpeedMultiplier << "x / " << SpeedMultiplier*4 << "\n";
	ss << "\nLife: " << score_keeper->getLifebarAmount(LT_GROOVE) * 100.0 << "%";
	// ss << "\nt / st " << std::setiosflags(std::ios::fixed) << SongTime << " / " << SongTimeReal << " / " << Music->GetPlayedTime() << std::resetiosflags(std::ios::fixed);

#ifdef DEBUG
	ss << "\nVert: " << CurrentVertical;
	// ss << "\ntotal notes:  " << [total notes];
	ss << "\nloaded notes:  " << CurrentDiff->TotalScoringObjects;
	ss << "\nholds hit: " << holds_hit;
	ss << "\nholds missed: " << holds_missed;
	ss << "\nloaded holds: " << CurrentDiff->TotalHolds;
	ss << "\nnotes hit: " << std::setprecision(2) << float(score_keeper->getScore(ST_NOTES_HIT)) / CurrentDiff->TotalScoringObjects * 100.0 << "%";	
	ss << "\ncombo: " << std::resetiosflags(std::ios::fixed) << score_keeper->getScore(ST_NOTES_HIT);
	ss << "\nmax combo: " << score_keeper->getScore(ST_MAX_COMBO);
#endif

	GFont->DisplayText(ss.str().c_str(), Vec2(0,0));

	if (!Active)
		GFont->DisplayText("press 'enter' to start", Vec2( ScreenWidth / 2 - 23 * 3,ScreenHeight * 5/8));

	for (unsigned int i = 0; i < Channels; i++)
	{
		std::stringstream ss;
		ss << lastClosest[i];
		GFont->DisplayText(ss.str().c_str(), Keys[i].GetPosition());
	}

	Animations->DrawFromLayer(14);

	return Running;
}
