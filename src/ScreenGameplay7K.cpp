#include <iomanip>
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

BitmapFont * GFont = NULL;

using namespace VSRG;

ScreenGameplay7K::ScreenGameplay7K()
{
	SpeedMultiplier = 0;
	SongOldTime = -1;
	Music = NULL;
	MissSnd = NULL;
	GameTime = 0;

	waveEffectEnabled = false;
	waveEffect = 0;
	WaitingTime = 1.5;

	NoFail = true;
	SelectedHiddenMode = HIDDENMODE_NONE; // No Hidden
	RealHiddenMode = HIDDENMODE_NONE;
	HideClampSum = 0;

	Auto = false;

	SpeedMultiplierUser = 4;

	CurrentVertical = 0;
	SongTime = SongTimeReal = 0;
	beatScrollEffect = 0;
	Channels = 0;

	AudioCompensation = (Configuration::GetConfigf("AudioCompensation") != 0);
	TimeCompensation = 0;

	InterpolateTime = (Configuration::GetConfigf("InterpolateTime") != 0);

	if (!GFont)
	{
		GFont = new BitmapFont();
		GFont->LoadSkinFontImage("font.tga", Vec2(6, 15), Vec2(8, 16), Vec2(6, 15), 0);
	}

	MissTime = 0;
	LoadedSong = NULL;
	Active = false;
}

void ScreenGameplay7K::Cleanup()
{
	CurrentDiff->Destroy();

	if (LoadedSong)
		delete LoadedSong;

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
		float LimPos = - ((JudgmentLinePos / ScreenHeight)*2 - 1); // Frac. of screen
		float AdjustmentSize;

		if (Upscroll)
		{
			Center = -(( ((ScreenHeight - JudgmentLinePos) / 2 + JudgmentLinePos) / ScreenHeight)*2 - 1);

			AdjustmentSize = -( ((ScreenHeight - JudgmentLinePos) / 2 / ScreenHeight) - 1 ); // A quarter of the playing field.

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
			Center = -((JudgmentLinePos / 2 / ScreenHeight)*2 - 1);

			AdjustmentSize = -( ((JudgmentLinePos) / 2 / ScreenHeight) - 1 ); // A quarter of the playing field.

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

void ScreenGameplay7K::Init(Song* S, int DifficultyIndex, const ScreenGameplay7K::Parameters &Param)
{
	MySong = S;
	CurrentDiff = S->Difficulties[DifficultyIndex];

	Upscroll = Param.Upscroll;
	StartMeasure = Param.StartMeasure;
	waveEffectEnabled = Param.Wave;
	SelectedHiddenMode = Param.HiddenMode;
	Preloaded = Param.Preloaded;
	Auto = Param.Auto;

	Animations = new GraphObjectMan();

	score_keeper = new ScoreKeeper7K();
}

void ScreenGameplay7K::RecalculateMatrix()
{
	PositionMatrix = glm::translate(Mat4(), glm::vec3(0, JudgmentLinePos + CurrentVertical * SpeedMultiplier, 0));
	PositionMatrixJudgment = glm::translate(Mat4(), glm::vec3(0, JudgmentLinePos, 0));

	for (uint8 i = 0; i < Channels; i++)
		NoteMatrix[i] = glm::translate(Mat4(), glm::vec3(LanePositions[i], 0, 14)) * noteEffectsMatrix[i] *  glm::scale(Mat4(), glm::vec3(LaneWidth[i], NoteHeight, 1));
}

void ScreenGameplay7K::LoadThreadInitialization()
{
	MissSnd = new SoundSample();
	if (MissSnd->Open((GameState::GetInstance().GetSkinPrefix() + "miss.ogg").c_str()))
		MixerAddSample(MissSnd);
	else
		delete MissSnd;

	if (AudioCompensation)
		TimeCompensation = MixerGetLatency();

	// The difficulty details are destroyed; which means we should load this from its original file.
	if (!Preloaded)
	{
		/* Load song from directory */
		SongLoader Loader(GameState::GetInstance().GetSongDatabase());
		Directory FN;

		Log::Printf("Loading Chart...");
		LoadedSong = Loader.LoadFromMeta(MySong, CurrentDiff, &FN);

		if (LoadedSong == NULL)
		{
			Log::Printf("Failure to load chart. (Filename: %s)\n", FN.path().c_str());
			DoPlay = false;
			return;
		}

		MySong = LoadedSong;

		/*
			At this point, MySong == LoadedSong, which means it's not a metadata-only Song* Instance.
			The old copy is preserved; but this new one will be removed by the end of ScreenGameplay7K.
		*/
	}

	// Now that the song is loaded, let's try and load the proper audio file.
	if (!Music)
	{
		Music = new AudioStream();
		if (Music->Open((MySong->SongDirectory + MySong->SongFilename).c_str()))
		{
			MixerAddStream(Music);
		}
		else
		{
			if (!CurrentDiff->IsVirtual)
			{
				// Caveat: Try to autodetect an mp3/ogg file.
				std::vector<String> DirCnt;
				Directory SngDir = MySong->SongDirectory;

				SngDir.ListDirectory(DirCnt, Directory::FS_REG);
				for (std::vector<String>::iterator i = DirCnt.begin();
					i != DirCnt.end();
					i++)
				{
					if (Directory(*i).GetExtension() == "mp3" || Directory(*i).GetExtension() == "ogg")
						if ( Music->Open( (SngDir / *i ).c_path()) )
						{
							MixerAddStream(Music);
							goto MusicWasLoaded;
						}
				}

				delete Music;
				Music = NULL;

				Log::Printf("Unable to load song (Filename: %s)\n", MySong->SongFilename.c_str());
				DoPlay = false;
				return; // Quit.
			}
		}
	}

MusicWasLoaded:

	// What, you mean we don't have timing data at all?
	if (CurrentDiff->Timing.size() == 0)
	{
		Log::Printf("Error loading chart: No timing data.\n");
		DoPlay = false;
		return;
	}

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

	Log::Printf("Processing song... ");

	if (DesiredDefaultSpeed)
	{

		if (Type == SPEEDTYPE_CMOD) // cmod
		{
			SpeedMultiplierUser = 1;
			SpeedConstant = DesiredDefaultSpeed;
		}

		MySong->Process(CurrentDiff, NotesByChannel, Drift, SpeedConstant);

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
		}else if (Type == SPEEDTYPE_FIRST) // We use this case as default. The logic is "Not a CMod, Not a MMod, then use first, the default.
		{
			double DesiredMultiplier =  DesiredDefaultSpeed / CurrentDiff->VerticalSpeeds[0].Value;

			SpeedMultiplierUser = DesiredMultiplier;
		}else if (Type != SPEEDTYPE_CMOD) // other cases
		{
			double bpsd = 4.0/(CurrentDiff->BPS[0].Value);
			double Speed = (MeasureBaseSpacing / bpsd);
			double DesiredMultiplier = DesiredDefaultSpeed / Speed;

			SpeedMultiplierUser = DesiredMultiplier;
		}

	}else
		MySong->Process(CurrentDiff, NotesByChannel, Drift); // Regular processing

	Channels = CurrentDiff->Channels;
	VSpeeds = CurrentDiff->VerticalSpeeds;

	Log::Printf("Loading samples... ");

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

	if (Configuration::GetConfigf("DisableBMP") == 0)
	{
		if (CurrentDiff->BMPList.size())
			Log::Printf("Loading BMPs...\n");

		for (std::map<int, String>::iterator i = CurrentDiff->BMPList.begin(); i != CurrentDiff->BMPList.end(); i++)
		{
			BMPs.AddToListIndex(i->second, MySong->SongDirectory, i->first);
		}

		// We don't need this any more.
		CurrentDiff->BMPList.clear();

		BMPEvents = CurrentDiff->BMPEvents;
		BMPEventsMiss = CurrentDiff->BMPEventsMiss;
		BMPEventsLayer = CurrentDiff->BMPEventsLayer;
		BMPEventsLayer2 = CurrentDiff->BMPEventsLayer2;
	}


	Log::Printf("Done.\n");

	// Get Noteheight
	NoteHeight = Configuration::GetSkinConfigf("NoteHeight");

	if (!NoteHeight)
		NoteHeight = 10;

	// Get Gear Height
	char str[256];
	char nstr[256];

	sprintf(nstr, "Channels%d", CurrentDiff->Channels);

	sprintf(str, "GearHeight");
	GearHeightFinal = Configuration::GetSkinConfigf(str, nstr);

	/* Initial object distance */
	if (!Upscroll)
		JudgmentLinePos = float(ScreenHeight) - GearHeightFinal;
	else
		JudgmentLinePos = GearHeightFinal;

	JudgmentLinePos += (Upscroll ? NoteHeight/2 : -NoteHeight/2);
	CurrentVertical = IntegrateToTime (VSpeeds, -WaitingTime);
	CurrentBeat = IntegrateToTime(CurrentDiff->BPS, 0);

	RecalculateMatrix();
	MultiplierChanged = true;

	BarlineEnabled = Configuration::GetSkinConfigf("BarlineEnabled");
	BarlineOffsetKind = Configuration::GetSkinConfigf("BarlineOffset");
	BarlineX = Configuration::GetSkinConfigf("BarlineX");
	BarlineWidth = Configuration::GetSkinConfigf("BarlineWidth");

	if (BarlineEnabled)
	{
		CurrentDiff->GetMeasureLines(MeasureBarlines, TimeCompensation);

		int UpscrollMod = Upscroll ? -1 : 1;
		BarlineOffset = BarlineOffsetKind == 0 ? NoteHeight * UpscrollMod / 2 : 0;
	}

	ErrorTolerance = Configuration::GetConfigf("ErrorTolerance");

	if (ErrorTolerance <= 0)
		ErrorTolerance = 5; // ms

	// This will execute the script once, so we won't need to do it later
	SetupScriptConstants();
	Animations->Preload(GameState::GetInstance().GetSkinPrefix() + "screengameplay7k.lua", "Preload");
	score_keeper->setMaxNotes(CurrentDiff->TotalScoringObjects);
	DoPlay = true;
}

void ScreenGameplay7K::SetupScriptConstants()
{
	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("Upscroll", Upscroll);
	L->SetGlobal("Channels", Channels);
	L->SetGlobal("JudgmentLineY", JudgmentLinePos);
	L->SetGlobal("Auto", Auto);
	L->SetGlobal("AccuracyHitMS", score_keeper->getMissCutoff());
	L->SetGlobal("SongDuration", CurrentDiff->Duration);
	L->SetGlobal("SongDurationBeats", BeatAtTime(CurrentDiff->BPS, CurrentDiff->Duration, CurrentDiff->Offset + TimeCompensation));
	L->SetGlobal("WaitingTime", WaitingTime);
	L->SetGlobal("Beat", CurrentBeat);
	L->SetGlobal("Lifebar", score_keeper->getLifebarAmount(LT_GROOVE));

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
		GearLaneImage[i] = GameState::GetInstance().GetSkinImage(str);

		sprintf(str, "key%dd.png", (int)GetSkinConfigf(cstr, nstr));
		GearLaneImageDown[i] = GameState::GetInstance().GetSkinImage(str);

		sprintf(str, "Key%dX", i+1);
		LanePositions[i] = Configuration::GetSkinConfigf(str, nstr);

		sprintf(str, "Key%dWidth", i+1);
		LaneWidth[i] = Configuration::GetSkinConfigf(str, nstr);

		Keys[i].SetImage ( GearLaneImage[i] );
		Keys[i].SetSize( LaneWidth[i], GearHeightFinal );
		Keys[i].Centered = true;

		float UMod = (Upscroll? -1:1);

		Keys[i].SetPosition( LanePositions[i], JudgmentLinePos + UMod * GearHeightFinal/2 + UMod * NoteHeight/2);

		if (Upscroll)
			Keys[i].SetRotation(180);

		Keys[i].SetZ(15);
	}

	if (BarlineEnabled)
		Barline = new Line();
}

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

void ScreenGameplay7K::MainThreadInitialization()
{
	if (!DoPlay) // Failure to load something important?
	{
		Running = false;
		return;
	}

	PlayReactiveSounds = (CurrentDiff->IsVirtual || !(Configuration::GetConfigf("DisableHitsounds")));

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
		NoteImages[i] = GameState::GetInstance().GetSkinImage(Filename);

		/* Hold image */
		sprintf(cstr, "Key%dHoldImage", i+1);

		Filename = Configuration::GetSkinConfigs(cstr, nstr);
		NoteImagesHold[i] = GameState::GetInstance().GetSkinImage(Filename);

		lastClosest[i] = 0;

		HeldKey[i] = NULL;
	}

	NoteImage = GameState::GetInstance().GetSkinImage("note.png");

	Image* BackgroundImage = ImageLoader::Load(MySong->SongDirectory + MySong->BackgroundFilename);

	if (BackgroundImage)
		Background.SetImage(BackgroundImage);
	else
	{
		// Caveat 2: Try to automatically load background.
		std::vector<String> DirCnt;
		Directory SngDir = MySong->SongDirectory;

		SngDir.ListDirectory(DirCnt, Directory::FS_REG);
		for (std::vector<String>::iterator i = DirCnt.begin();
			i != DirCnt.end();
			i++)
		{
			String ext = Directory(*i).GetExtension();
			if (strstr(i->c_str(), "bg") && (ext == "jpg" || ext == "png"))
				if (BackgroundImage = ImageLoader::Load(MySong->SongDirectory + *i))
					break;
		}

		if (!BackgroundImage)
			Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("DefaultGameplay7KBackground")));
		else
			Background.SetImage(BackgroundImage);
	}

	Background.SetZ(0);
	Background.AffectedByLightning = true;

	if (Background.GetImage())
	{
		float SizeRatio = ScreenHeight / Background.GetHeight();
		Background.SetScale(SizeRatio);
		Background.Centered = true;
		Background.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	}

	Layer1.Centered = Layer2.Centered = LayerMiss.Centered = true;
	float LW = Background.GetWidth(), LH = Background.GetHeight();
	float Scale = Background.GetScale().x;
	Layer1.SetSize(LW, LH); Layer2.SetSize(LW, LH); LayerMiss.SetSize(LW, LH);
	Layer1.BlackToTransparent = Layer2.BlackToTransparent = true;
	Layer1.SetScale(Scale); Layer2.SetScale(Scale); LayerMiss.SetScale(Scale);
	Layer1.SetZ(0); Layer2.SetZ(0); LayerMiss.SetZ(0);
	Layer1.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	Layer2.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	LayerMiss.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	LayerMiss.SetImage(BMPs.GetFromIndex(0));


	WindowFrame.SetLightMultiplier(0.75f);

	Animations->Initialize("", false);

	memset(PlaySounds, 0, sizeof(PlaySounds));

	CalculateHiddenConstants();
	AssignMeasure(StartMeasure);

	if (!StartMeasure)
		WaitingTime = abs(std::min(-WaitingTime, CurrentDiff->Offset - 1.5));
	else
		WaitingTime = 0;


	CurrentBeat = BeatAtTime(CurrentDiff->BPS, -WaitingTime, CurrentDiff->Offset + TimeCompensation);
	Animations->GetImageList()->ForceFetch();

	BMPs.LoadAll();
// 	BMPs.ForceFetch();
	Running = true;
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
				Animations->GetEnv()->CallFunction("OnActivate");
				Animations->GetEnv()->RunFunction();
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

void ScreenGameplay7K::RunAutoEvents()
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

	// Play BMP Base Events
	for (std::vector<AutoplayBMP>::iterator b = BMPEvents.begin();
		b != BMPEvents.end();
		b++)
	{
		if (b->Time <= SongTime)
		{
			Image* Img = BMPs.GetFromIndex(b->BMP);
			if (Img != NULL)
			{
				Background.SetImage(Img, false);
			}

			b = BMPEvents.erase(b);
			if (b == BMPEvents.end()) break;
		}
	}

	// BMP Miss layer events
	for (std::vector<AutoplayBMP>::iterator b = BMPEventsMiss.begin();
		b != BMPEventsMiss.end();
		b++)
	{
		if (b->Time <= SongTime)
		{
			Image* Img = BMPs.GetFromIndex(b->BMP);
			if (Img != NULL)
				LayerMiss.SetImage(Img, false);

			b = BMPEventsMiss.erase(b);
			if (b == BMPEventsMiss.end()) break;
		}
	}

	// BMP Layer1 events
	for (std::vector<AutoplayBMP>::iterator b = BMPEventsLayer.begin();
		b != BMPEventsLayer.end();
		b++)
	{
		if (b->Time <= SongTime)
		{
			Image* Img = BMPs.GetFromIndex(b->BMP);
			if (Img != NULL)
				Layer1.SetImage(Img, false);

			b = BMPEventsLayer.erase(b);
			if (b == BMPEventsLayer.end()) break;
		}
	}

	// BMP Layer 2 events.
	for (std::vector<AutoplayBMP>::iterator b = BMPEventsLayer2.begin();
		b != BMPEventsLayer2.end();
		b++)
	{
		if (b->Time <= SongTime)
		{
			Image* Img = BMPs.GetFromIndex(b->BMP);
			if (Img != NULL)
				Layer2.SetImage(Img, false);

			b = BMPEventsLayer2.erase(b);
			if (b == BMPEventsLayer2.end()) break;
		}
	}
}

void ScreenGameplay7K::CheckShouldEndScreen()
{
	// Reached the end!
	if (SongTime > CurrentDiff->Duration + 3)
	{
		ScreenEvaluation7K *Eval = new ScreenEvaluation7K(this);
		Eval->Init(score_keeper);
		Next = Eval;
	}

	// We failed!
	if (score_keeper->getLifebarAmount(LT_GROOVE) <= 0 && !NoFail)
		Running = false;
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
