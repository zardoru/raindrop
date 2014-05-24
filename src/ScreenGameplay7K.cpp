#include "GameGlobal.h"
#include "Screen.h"
#include "Audio.h"
#include "FileManager.h"
#include "ImageLoader.h"
#include "GraphObject2D.h"
#include "BitmapFont.h"
#include "GameWindow.h"

#include <iomanip>

#include "LuaManager.h"
#include "GraphObjectMan.h"

#include "ScoreKeeper.h"
#include "ScreenGameplay7K.h"
#include "ScreenEvaluation7K.h"

BitmapFont * GFont = NULL;

using namespace VSRG;

ScreenGameplay7K::ScreenGameplay7K()
{
	SpeedMultiplier = 0;
	SongOldTime = -1;
	Music = NULL;
	deltaPos = 0;

	waveEffectEnabled = false;
	waveEffect = 0;
	WaitingTime = 1.5;

	NoFail = true;
	SelectedHiddenMode = HIDDENMODE_NONE; // No Hidden
	RealHiddenMode = HIDDENMODE_NONE;
	HideClampSum = 0;

	SpeedMultiplierUser = 4;

	CurrentVertical = 0;
	SongTime = SongTimeReal = 0;
	beatScrollEffect = 0;

	AudioCompensation = (Configuration::GetConfigf("AudioCompensation") != 0);
	TimeCompensation = 0;

	InterpolateTime = (Configuration::GetConfigf("InterpolateTime") != 0);

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

	for (std::map<int, SoundSample*>::iterator i = Keysounds.begin(); i != Keysounds.end(); i++)
	{
		MixerRemoveSample(i->second);
		delete i->second;
	}

	MixerRemoveSample(MissSnd);

	delete MissSnd;
	delete Animations;
	delete score_keeper;
}

void ScreenGameplay7K::CalculateHiddenConstants()
{
	/*
		Given the top of the screen being 1, the bottom being -1
		calculate the range for which the current hidden mode is defined.
	*/
	const float FlashlightRatio = 0.25;
	float Center;

	// Hidden calc 
	if (SelectedHiddenMode)
	{
		float LimPos = - ((BasePos / ScreenHeight)*2 - 1); // Frac. of screen
		float AdjustmentSize;

		if (Upscroll)
		{
			Center = -(( ((ScreenHeight - BasePos) / 2 + BasePos) / ScreenHeight)*2 - 1);
			
			AdjustmentSize = -( ((ScreenHeight - BasePos) / 2 / ScreenHeight) - 1 ); // A quarter of the playing field.

			if (SelectedHiddenMode == 2)
			{
				HideClampHigh = Center;
				HideClampLow = -1 + AdjustmentSize;
			}else if (SelectedHiddenMode == 1)
			{
				HideClampHigh = LimPos - AdjustmentSize;
				HideClampLow = Center;
			}

			// Invert Hidden Mode.
			if (SelectedHiddenMode == HIDDENMODE_SUDDEN) RealHiddenMode = HIDDENMODE_HIDDEN;
			else if (SelectedHiddenMode == HIDDENMODE_HIDDEN) RealHiddenMode = HIDDENMODE_SUDDEN;
			else RealHiddenMode = SelectedHiddenMode;
		}else
		{
			Center = -((BasePos / 2 / ScreenHeight)*2 - 1);
			
			AdjustmentSize = -( ((BasePos) / 2 / ScreenHeight) - 1 ); // A quarter of the playing field.

			// Hidden/Sudden
			if (SelectedHiddenMode == 2)
			{
				HideClampHigh = 1 - AdjustmentSize;
				HideClampLow = Center;
			}else if (SelectedHiddenMode == 1)
			{
				HideClampHigh = Center;
				HideClampLow = LimPos + AdjustmentSize;
			}

			RealHiddenMode = SelectedHiddenMode;
		}

		if (SelectedHiddenMode == HIDDENMODE_FLASHLIGHT) // Flashlight
		{
			HideClampLow = Center - FlashlightRatio;
			HideClampHigh = Center + FlashlightRatio;
			HideClampSum = - Center;
			HideClampFactor = 1 / FlashlightRatio;
		}else // Hidden/Sudden
		{
			HideClampSum = - HideClampLow;
			HideClampFactor = 1 / (HideClampHigh + HideClampSum);
		}
	}
}

void ScreenGameplay7K::Init(Song* S, int DifficultyIndex, bool UseUpscroll)
{
	MySong = S;
	CurrentDiff = S->Difficulties[DifficultyIndex];
	Upscroll = UseUpscroll;

	Animations = new GraphObjectMan();

	score_keeper = new ScoreKeeper7K();
	score_keeper->setMaxNotes(CurrentDiff->TotalScoringObjects);
}

void ScreenGameplay7K::RecalculateMatrix()
{
	PositionMatrix = glm::translate(Mat4(), glm::vec3(0, BasePos + CurrentVertical * SpeedMultiplier + deltaPos, 0));
	PositionMatrixJudgement = glm::translate(Mat4(), glm::vec3(0, BasePos + deltaPos, 0));

	for (uint8 i = 0; i < Channels; i++)
		NoteMatrix[i] = glm::translate(Mat4(), glm::vec3(LanePositions[i], 0, 14)) * noteEffectsMatrix[i] *  glm::scale(Mat4(), glm::vec3(LaneWidth[i], NoteHeight, 1));
}

void ScreenGameplay7K::LoadThreadInitialization()
{
	MissSnd = new SoundSample();
	if (MissSnd->Open((FileManager::GetSkinPrefix() + "miss.ogg").c_str()))
		MixerAddSample(MissSnd);

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
	
	if (!Music)
	{
		Music = new AudioStream();
		Music->Open((MySong->SongDirectory + MySong->SongFilename).c_str());
		MixerAddStream(Music);
	}

	if (AudioCompensation)
		TimeCompensation = MixerGetLatency();

	TimeCompensation += Configuration::GetConfigf("Offset7K");

	double DesiredDefaultSpeed = Configuration::GetSkinConfigf("DefaultSpeedUnits");
	double Drift = TimeCompensation;

	ESpeedType Type = (ESpeedType)(int)Configuration::GetSkinConfigf("DefaultSpeedKind");
	double SpeedConstant = 0; // Unless set, assume we're using speed changes

	/* 
 * 		There are three kinds of speed modifiers:
 * 			-CMod (Keep speed the same through the song, equal to a constant)
 * 			-MMod (Find highest speed and set multiplier to such that the highest speed is equal to a constant)
 *			-First (Find the first speed in the chart, and set multiplier to such that the first speed is equal to a constant)
 *
 *			The calculations are done ahead, and while SpeedConstant = 0 either MMod or first are assumed
 *			but only if there's a constant specified by the user.
 * */

	wprintf(L"Processing song... ");

	if (DesiredDefaultSpeed)
	{

		if (Type == SPEEDTYPE_CMOD) // cmod
		{
			SpeedMultiplierUser = 1;
			SpeedConstant = DesiredDefaultSpeed;
		}

		MySong->Process(CurrentDiff, NotesByMeasure, Drift, SpeedConstant);

		if (Type == SPEEDTYPE_MMOD) // mmod
		{
			double max = 0; // Find the highest speed
			for (TimingData::iterator i = CurrentDiff->VerticalSpeeds.begin();
				i != CurrentDiff->VerticalSpeeds.end();
				i++)
			{
				max = std::max(max, abs(i->Value));
			}
		
			double Ratio = DesiredDefaultSpeed / max; // How much above or below are we from the maximum speed?
			SpeedMultiplierUser = Ratio;
		}else if (Type != SPEEDTYPE_CMOD) // We use this case as default. The logic is "Not a CMod, Not a MMod, then use first, the default.
		{
			double DesiredMultiplier =  DesiredDefaultSpeed / CurrentDiff->VerticalSpeeds[0].Value;

			SpeedMultiplierUser = DesiredMultiplier;
		}

	}else
		MySong->Process(CurrentDiff, NotesByMeasure, Drift); // Regular processing

	wprintf(L"Copying data... ");
	Channels = CurrentDiff->Channels;
	VSpeeds = CurrentDiff->VerticalSpeeds;

	wprintf(L"Loading samples... ");

	for (std::map<int, String>::iterator i = CurrentDiff->SoundList.begin(); i != CurrentDiff->SoundList.end(); i++)
	{
		Keysounds[i->first] = new SoundSample();

#ifdef WIN32

		std::wstring sd = Utility::Widen(MySong->SongDirectory) + L"/" + Utility::Widen(i->second);

		if (Keysounds[i->first]->Open(Utility::Narrow(sd).c_str()))
			MixerAddSample(Keysounds[i->first]);

#else
		if (Keysounds[i->first]->Open((MySong->SongDirectory + "/" + i->second).c_str()))
			MixerAddSample(Keysounds[i->first]);
#endif
	}

	BGMEvents = CurrentDiff->BGMEvents;

	wprintf(L"Done.\n");

	NoteHeight = Configuration::GetSkinConfigf("NoteHeight");

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

	// BasePos = JudgementLinePos + (Upscroll ? NoteHeight/2 : -NoteHeight/2);
	BasePos = JudgementLinePos + (Upscroll ? NoteHeight/2 : -NoteHeight/2);
	CurrentVertical -= VSpeeds.at(0).Value * (WaitingTime);

	RecalculateMatrix();
	MultiplierChanged = true;

	ErrorTolerance = Configuration::GetConfigf("ErrorTolerance");

	if (ErrorTolerance <= 0)
		ErrorTolerance = 5; // ms
}

void ScreenGameplay7K::SetupScriptConstants()
{
	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("Upscroll", Upscroll);
	L->SetGlobal("Channels", Channels);
	L->SetGlobal("JudgementLineY", JudgementLinePos);
	L->SetGlobal("AccuracyHitMS", score_keeper->getAccMax());
	L->SetGlobal("SongDuration", CurrentDiff->Duration);
	L->SetGlobal("SongDurationBeats", BeatAtTime(CurrentDiff->BPS, CurrentDiff->Duration, CurrentDiff->Offset + TimeCompensation));
	L->SetGlobal("WaitingTime", WaitingTime);

	Animations->AddLuaTarget(&Background, "ScreenBackground");
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

	Image* BackgroundImage = ImageLoader::Load(MySong->SongDirectory + MySong->BackgroundFilename);

	if (BackgroundImage)
		Background.SetImage(BackgroundImage);
	else
		Background.SetImage(ImageLoader::LoadSkin(Configuration::GetSkinConfigs("DefaultGameplay7KBackground")));

	Background.SetZ(0);
	Background.AffectedByLightning = true;

	if (Background.GetImage())
	{
		float SizeRatio = ScreenHeight / Background.GetHeight();
		Background.SetScale(SizeRatio);
		Background.Centered = true;
		Background.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	}

	WindowFrame.SetLightMultiplier(0.75f);

	SetupScriptConstants();
	Animations->Initialize( FileManager::GetSkinPrefix() + "screengameplay7k.lua" );

	memset(PlaySounds, 0, sizeof(PlaySounds));

	CurrentBeat = BeatAtTime(CurrentDiff->BPS, -1.5, CurrentDiff->Offset + TimeCompensation);

	CalculateHiddenConstants();
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
			HideClampSum -= 0.1;
			break;
		case KT_Right:
			HideClampSum += 0.1;
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
	L->SetGlobal("SpeedMultiplier", SpeedMultiplier);
	L->SetGlobal("SpeedMultiplierUser", SpeedMultiplierUser);
	L->SetGlobal("waveEffectEnabled", waveEffectEnabled);
	L->SetGlobal("Active", Active);
	L->SetGlobal("SongTime", SongTime);
	L->SetGlobal("Lifebar", score_keeper->getLifebarAmount(LT_GROOVE));

	CurrentBeat = BeatAtTime(CurrentDiff->BPS, SongTime, CurrentDiff->Offset + TimeCompensation);
	L->SetGlobal("Beat", CurrentBeat);

	L->NewArray();

	for (uint32 i = 0; i < Channels; i++)
	{
		L->SetFieldI(i + 1, HeldKey[i]);
	}

	L->FinalizeArray("HeldKeys");
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

bool ScreenGameplay7K::Run(double Delta)
{
	float SongDelta;

	if (Next)
		return RunNested(Delta);

	if (Active)
	{
		ScreenTime += Delta;

		if (ScreenTime >= WaitingTime)
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

			if (Music->IsPlaying() && !CurrentDiff->IsVirtual)
			{
				SongDelta = Music->GetStreamedTime() - SongOldTime;
				SongTimeReal += SongDelta;

				if ( (SongDelta > 0.001 && abs(SongTime - SongTimeReal) * 1000 > ErrorTolerance) || !InterpolateTime ) // Significant delta with a x ms difference? We're pretty off..
					SongTime = SongTimeReal;
			}

			// Play BGM events.
			for (std::vector<AutoplaySound>::iterator s = BGMEvents.begin(); s != BGMEvents.end(); s++)
			{
				if (s->Time <= SongTime)
				{
					if (Keysounds[s->Sound])
						Keysounds[s->Sound]->Play();
					s = BGMEvents.erase(s);
					if (s == BGMEvents.end()) break;
				}
			}

			CurrentVertical = IntegrateToTime(VSpeeds, SongTime);
			RunMeasures();

			SongOldTime = SongTimeReal;

			if (SongTime > CurrentDiff->Duration + 3)
			{
				ScreenEvaluation7K *Eval = new ScreenEvaluation7K(this);
				Eval->Init(score_keeper);
				Next = Eval;
			}

			if (score_keeper->getLifebarAmount(LT_GROOVE) <= 0 && !NoFail)
				Running = false;
		}else
		{
			SongTime = -(WaitingTime - ScreenTime);
			SongDelta = 0;
			CurrentVertical = IntegrateToTime(VSpeeds, SongTime);
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


	ss << "\nMult/Speed: " << std::setprecision(2) << std::setiosflags(std::ios::fixed) << SpeedMultiplier << "x / " << SpeedMultiplier*4;
	if (SongTime > 0)
		ss << "\nScrolling Speed: " << SectionValue(VSpeeds, SongTime) * SpeedMultiplier;
	else
		ss << "\nScrolling Speed: " << SectionValue(VSpeeds, 0) * SpeedMultiplier;

	GFont->DisplayText(ss.str().c_str(), Vec2(0, ScreenHeight - 45));

	if (!Active)
		GFont->DisplayText("press 'enter' to start", Vec2( ScreenWidth / 2 - 23 * 3,ScreenHeight * 5/8));

	for (unsigned int i = 0; i < Channels; i++)
	{
		std::stringstream ss;
		ss << lastClosest[i];
		GFont->DisplayText(ss.str().c_str(), Keys[i].GetPosition() - Vec2(DigitCount(lastClosest[i]) * 3, 7));
	}

	Animations->DrawFromLayer(14);

	return Running;
}
