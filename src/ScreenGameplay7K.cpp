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
#include "ScreenGameplay7K.h"

BitmapFont * GFont = NULL;
VorbisSample *MissSnd = NULL;
int lastPressed = 0;
int lastMsOff[MAX_CHANNELS];
int lastClosest[MAX_CHANNELS];

/* Time before actually starting everything. */
#define WAITING_TIME 1.5
#define EQ(x) (-100.0/84.0)*x + 10000.0/84.0

#define ACC_MIN 16
#define ACC_MIN_SQ ACC_MIN * ACC_MIN
#define ACC_MAX 100
#define ACC_MAX_SQ ACC_MAX * ACC_MAX
#define ACC_CUTOFF 135

int holds_missed = 0;
int holds_hit = 0;

float accuracy_percent(float var){
	//if(var < ACC_MIN_SQ) return 100;
	//if(var > ACC_MAX_SQ) return 0;

	return (ACC_MAX_SQ - var) / (ACC_MAX_SQ - ACC_MIN_SQ) * 100;
}


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
		Music->Stop();

	delete Animations;
}

void ScreenGameplay7K::Init(Song7K* S, int DifficultyIndex, bool UseUpscroll)
{
	MySong = S;
	CurrentDiff = S->Difficulties[DifficultyIndex];
	Upscroll = UseUpscroll;

	holds_hit = 0;
	holds_missed = 0;

	Animations = new GraphObjectMan();
}

void ScreenGameplay7K::RecalculateEffects()
{
	float SongTime = 0;

	if (Music)
		SongTime = Music->GetPlaybackTime();

	if (waveEffectEnabled)
	{
		waveEffect = sin(SongTime) * 0.5 * SpeedMultiplierUser;
		MultiplierChanged = true;
	}
}

#define CLAMP(var, min, max) (var) < (min) ? (min) : (var) > (max) ? (max) : (var)

void ScreenGameplay7K::HitNote (double TimeOff, uint32 Lane)
{
	Score.total_sqdev += TimeOff * TimeOff;
	Score.TotalNotes++;
	Score.notes_hit++;
	if(TimeOff > ACC_MAX) Score.combo = 0; else ++Score.combo;
	if(Score.combo > Score.max_combo) Score.max_combo = Score.combo;

	Score.Accuracy = accuracy_percent(Score.total_sqdev / Score.TotalNotes);

	Score.ex_score += TimeOff <= 20 ? 2 : TimeOff <= 40 ? 1 : 0;

	Animations->GetEnv()->CallFunction("HitEvent", 2);
	Animations->GetEnv()->PushArgument(TimeOff);
	Animations->GetEnv()->PushArgument((int)Lane + 1);
	Animations->GetEnv()->RunFunction();

	// scoring algorithm
	Score.dpScore += TimeOff <= ACC_MIN ? 2 : TimeOff <= ACC_MAX ? 1 : 0;
	Score.dpdpScore += Score.dpScore;
	Score.points = 1000000.0 * Score.dpdpScore / (CurrentDiff->TotalScoringObjects * (CurrentDiff->TotalScoringObjects + 1));

}

void ScreenGameplay7K::MissNote (double TimeOff, uint32 Lane)
{
	Score.total_sqdev += ACC_CUTOFF * ACC_CUTOFF;
	Score.TotalNotes++;
	Score.Accuracy = accuracy_percent(Score.total_sqdev / Score.TotalNotes);
	Score.combo = 0;

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
				if ((SongTime - m->GetTimeFinal()) * 1000 > ACC_CUTOFF && !m->WasNoteHit() && m->IsHold())
				{
					// remove hold notes that were never hit.
					MissNote((SongTime - m->GetTimeFinal()) * 1000, k);
					holds_missed += 1;
					m = (*i).MeasureNotes.erase(m);

					if (Score.combo > 10)
						MissSnd->Reset();

					if (m == (*i).MeasureNotes.end())
						break;
				}

				else if ((SongTime - m->GetStartTime()) * 1000 > ACC_CUTOFF && (!m->WasNoteHit() && m->IsEnabled()))
				{
					// remove notes that were never hit.
					MissNote((SongTime - m->GetStartTime()) * 1000, k);

					if (Score.combo > 10)
						MissSnd->Reset();
					
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

				if (tD < ACC_CUTOFF) /* Released in time */
				{
					HitNote(tD, Lane);
					holds_hit += 1;

					(*i).MeasureNotes.erase(m);

					lastClosest[Lane] = std::min(tD, (double)lastClosest[Lane]);

					return;
				}else /* Released off time */
				{
					MissNote(tD, Lane);
					holds_missed += 1;

					if (Score.combo > 10)
						MissSnd->Reset();

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

			if (tD > ACC_CUTOFF)
				goto next_note;
			else
			{

				if (tD < ACC_MAX) // Within hitting time, otherwise no feedback/miss feedback
				{
					HitNote(tD, Lane);
					if (m->IsHold() && m->IsEnabled())
					{
						holds_hit += 1;
						m->Hit();
						HeldKey[m->GetTrack()] = true;
					}
				}
				else
				{
					MissNote(tD, Lane);
					// missed feedback
					MissSnd->Reset();

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
			next_note: ;
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
		MissSnd = new VorbisSample((FileManager::GetSkinPrefix() + "miss.ogg").c_str());
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
		Music = new PaStreamWrapper(MySong->SongFilename.c_str());
	}

	if (AudioCompensation)
		TimeCompensation = GetDeviceLatency();

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
	L->SetGlobal("AccuracyHitMS", ACC_MAX);
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

	memset((void*)&Score, 0, sizeof (AccuracyData7K));
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
	L->SetGlobal("Combo", Score.combo);
	L->SetGlobal("MaxCombo", Score.max_combo);
	L->SetGlobal("SpeedMultiplier", SpeedMultiplier);
	L->SetGlobal("SpeedMultiplierUser", SpeedMultiplierUser);
	L->SetGlobal("waveEffectEnabled", waveEffectEnabled);
	L->SetGlobal("Active", Active);
}

bool ScreenGameplay7K::Run(double Delta)
{
	float SongDelta;
	
	UpdateScriptVariables();
	Animations->UpdateTargets(Delta);
	Animations->DrawUntilLayer(13);

	if (Active)
	{
		ScreenTime += Delta;

		if (ScreenTime > WAITING_TIME)
		{

			if (!Music || !Music->GetStream())
				return false; // Quit inmediately. There's no point.

			if (SongOldTime == -1)
			{
				Music->Start(false, false);
				SongOldTime = 0;
			}

			SongDelta = Music->GetStream()->GetStreamedTime() - SongOldTime;
			SongTime += SongDelta;

			CurrentVertical = VerticalAtTime(VSpeeds, SongTime);
			RunMeasures();

			SongOldTime = SongTime;

			/* Update music. */
			int32 r;
			Music->GetStream()->UpdateBuffer(r);
		}else
		{
			CurrentVertical += VSpeeds.at(0).Value * Delta; 
		}
	}

	RecalculateEffects();
	RecalculateMatrix();

	Background.Render();

	DrawMeasures();

	for (int32 i = 0; i < CurrentDiff->Channels; i++)
		Keys[i].Render();

	std::stringstream ss;

	ss << "score: " << int(Score.points);
	ss << "\naccuracy: " << std::setiosflags(std::ios::fixed) << std::setprecision(2) << Score.Accuracy << "%";
	ss << "\nnotes hit: " << std::setprecision(2) << float(Score.notes_hit) / CurrentDiff->TotalScoringObjects * 100.0 << "%";
	ss << "\nEX score: " << std::setprecision(2) << Score.ex_score / (CurrentDiff->TotalScoringObjects * 2.0) * 100.0 << "%";
	ss << "\ntotal notes:  " << Score.TotalNotes;
	ss << "\nloaded notes:  " << CurrentDiff->TotalScoringObjects;
	ss << "\nholds hit: " << holds_hit;
	ss << "\nholds missed: " << holds_missed;
	ss << "\nloaded holds: " << CurrentDiff->TotalHolds;
	ss << "\ncombo: " << std::resetiosflags(std::ios::fixed) << Score.combo;
	ss << "\nmax combo: " << Score.max_combo;
	ss << "\nMult/Speed: " << std::setiosflags(std::ios::fixed) << SpeedMultiplier << "x / " << SpeedMultiplier*4 << "\n";

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
