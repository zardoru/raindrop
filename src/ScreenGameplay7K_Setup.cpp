#include <cmath>
#include <fstream>

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
#include "SceneManager.h"

#include "ScoreKeeper7K.h"
#include "TrackNote.h"
#include "ScreenGameplay7K.h"
#include "ScreenGameplay7K_Mechanics.h"
#include "ScreenEvaluation7K.h"
#include "SongDatabase.h"

#include "AudioSourceOJM.h"

ScreenGameplay7K::ScreenGameplay7K()
{
	SpeedMultiplier = 0;
	SongOldTime = -1;
	Music = NULL;
	MissSnd = NULL;
	FailSnd = NULL;
	GameTime = 0;
	Speed = 1;

	waveEffectEnabled = false;
	waveEffect = 0;
	WaitingTime = 1.5;

	stage_failed = false;

	NoFail = true;

	SelectedHiddenMode = HIDDENMODE_NONE; // No Hidden
	RealHiddenMode = HIDDENMODE_NONE;
	HideClampSum = 0;

#if (defined WIN32) && (!defined NDEBUG)
	Auto = true;
#else
	Auto = false;
#endif

	perfect_auto = true;

	lifebar_type = LT_GROOVE;
	scoring_type = ST_IIDX;

	OJMAudio = NULL;
	
	SpeedMultiplierUser = 4;
	SongFinished = false;

	CurrentVertical = 0;
	SongTime = SongTimeReal = 0;
	beatScrollEffect = 0;

	AudioCompensation = (Configuration::GetConfigf("AudioCompensation") != 0);
	TimeCompensation = 0;

	InterpolateTime = (Configuration::GetConfigf("InterpolateTime") != 0);

	MissTime = 0;
	SuccessTime = 0;
	LoadedSong = NULL;
	Active = false;
}


void ScreenGameplay7K::Cleanup()
{
	CurrentDiff->Destroy();

	delete LoadedSong;

	if (Music)
	{
		MixerRemoveStream(Music);
		Music->Stop();
	}

	if (OJMAudio)
		delete OJMAudio;
	else
	{
		for (std::map<int, SoundSample*>::iterator i = Keysounds.begin(); i != Keysounds.end(); i++)
		{
			MixerRemoveSample(i->second);
			delete i->second;
		}
	}

	MixerRemoveSample(MissSnd);
	MixerRemoveSample(FailSnd);

	GameState::GetInstance().SetScorekeeper7K(nullptr);

	delete MissSnd;
	delete FailSnd;
	delete Animations;
	delete score_keeper;
}

void ScreenGameplay7K::AssignMeasure(uint32 Measure)
{
	double Beat = 0;

	if (!Measure)
	{
		Active = true;
		return;
	}

	for (uint32 i = 0; i < Measure; i++)
	{
		double Length = CurrentDiff->Data->Measures[i].MeasureLength;
		Beat += Length;
		// Log::Printf("Adding measure %d (%f): %f -> %f\n", i, Length, Beat - Length, Beat);
	}

	Log::Logf("Warping to measure measure %d at beat %f.\n", Measure, Beat);

	double Time = TimeAtBeat(CurrentDiff->Timing, CurrentDiff->Offset, Beat)
		+ StopTimeAtBeat(CurrentDiff->Data->StopsTiming, Beat);

	// Disable all notes before the current measure.
	for (uint32 k = 0; k < CurrentDiff->Channels; k++)
	{
		for (std::vector<VSRG::TrackNote>::iterator m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); )
		{
			if (m->GetStartTime() < Time)
			{
				m = NotesByChannel[k].erase(m);
				if (m == NotesByChannel[k].end()) break;
				else continue;
			}
			m++;
		}
	}

	// Remove non-played objects
	for (auto s = BGMEvents.begin(); s != BGMEvents.end();)
	{
		if (s->Time <= Time)
		{
			s = BGMEvents.erase(s);
			if (s == BGMEvents.end()) break;
			else continue;
		}
		s++;
	}

	SongTime = SongTimeReal = Time;

	if (Music)
	{
		Log::Printf("Setting stream to time %f.\n", Time);
		SongOldTime = -1;
		Music->SeekTime(Time);
	}

	Active = true;
}


void ScreenGameplay7K::Init(VSRG::Song* S, int DifficultyIndex, const ScreenGameplay7K::Parameters &Param)
{
	MySong = S;
	CurrentDiff = S->Difficulties[DifficultyIndex];

	Upscroll = Param.Upscroll;
	StartMeasure = Param.StartMeasure;
	waveEffectEnabled = Param.Wave;
	SelectedHiddenMode = Param.HiddenMode;
	Preloaded = Param.Preloaded;
	Auto = Param.Auto;

	Animations = new SceneManager("ScreenGameplay7K");
	score_keeper = new ScoreKeeper7K();
	GameState::GetInstance().SetScorekeeper7K(score_keeper);
	UpdateScriptScoreVariables();
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

bool ScreenGameplay7K::LoadChartData()
{
	if (!Preloaded)
	{
		// The difficulty details are destroyed; which means we should load this from its original file.
		SongLoader Loader(GameState::GetInstance().GetSongDatabase());
		Directory FN;

		Log::Printf("Loading Chart...");
		LoadedSong = Loader.LoadFromMeta(MySong, CurrentDiff, &FN);

		if (LoadedSong == NULL)
		{
			Log::Printf("Failure to load chart. (Filename: %s)\n", FN.path().c_str());
			return false;
		}

		MySong = LoadedSong;

		/*
			At this point, MySong == LoadedSong, which means it's not a metadata-only Song* Instance.
			The old copy is preserved; but this new one (LoadedSong) will be removed by the end of ScreenGameplay7K.
		*/
	}

	return true;
}

bool ScreenGameplay7K::LoadSongAudio()
{
	if (!Music)
	{
		Music = new AudioStream();
		Music->SetPitch(Speed);
		if (MySong->SongFilename.length() && Music->Open((MySong->SongDirectory + MySong->SongFilename).c_str()))
		{
			Log::Printf("Stream for %s succesfully opened.\n", MySong->SongFilename.c_str());
			MixerAddStream(Music);
		}
		else
		{
			if (!CurrentDiff->IsVirtual)
			{
				// Caveat: Try to autodetect an mp3/ogg file.
				std::vector<GString> DirCnt;
				Directory SngDir = MySong->SongDirectory;

				SngDir.ListDirectory(DirCnt, Directory::FS_REG);
				for (std::vector<GString>::iterator i = DirCnt.begin();
					i != DirCnt.end();
					i++)
				{
					if (Directory(*i).GetExtension() == "mp3" || Directory(*i).GetExtension() == "ogg")
						if ( Music->Open( (SngDir / *i ).c_path()) )
						{
							MixerAddStream(Music);
							return true;
						}
				}

				delete Music;
				Music = NULL;

				Log::Printf("Unable to load song (Path: %s)\n", MySong->SongFilename.c_str());
				DoPlay = false;
				return false; // Quit; couldn't find audio for a chart that requires it.
			}
		}
	}

	// Load samples.
	if (strstr(MySong->SongFilename.c_str(), ".ojm"))
	{
		Log::Printf("Loading OJM.\n");
		OJMAudio = new AudioSourceOJM;
		OJMAudio->Open((MySong->SongDirectory + MySong->SongFilename).c_str());

		for (int i = 0; i < 2000; i++)
		{
			SoundSample *Snd = OJMAudio->GetFromIndex(i);

			if (i != NULL)
				Keysounds[i] = Snd;
		}
	}
	else if (CurrentDiff->SoundList.size())
	{
		Log::Printf("Loading samples... ");
		for (std::map<int, GString>::iterator i = CurrentDiff->SoundList.begin(); i != CurrentDiff->SoundList.end(); i++)
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
	}

	BGMEvents = CurrentDiff->Data->BGMEvents;
	return true;
}

bool ScreenGameplay7K::ProcessSong()
{
	TimeCompensation = 0;

	double DesiredDefaultSpeed = Configuration::GetSkinConfigf("DefaultSpeedUnits");

	ESpeedType Type = (ESpeedType)(int)Configuration::GetSkinConfigf("DefaultSpeedKind");
	double SpeedConstant = 0; // Unless set, assume we're using speed changes

	int ApplyDriftVirtual = Configuration::GetConfigf("UseAudioCompensationKeysounds");
	int ApplyDriftDecoder = Configuration::GetConfigf("UseAudioCompensationNonKeysounded");
	
	if (AudioCompensation &&  // Apply drift is enabled and:
		((ApplyDriftVirtual && CurrentDiff->IsVirtual) ||  // We want to apply it to a keysounded file and it's virtual
		(ApplyDriftDecoder && !CurrentDiff->IsVirtual))) // or we want to apply it to a non-keysounded file and it's not virtual
		TimeCompensation += MixerGetLatency();

	if (!CurrentDiff->IsVirtual) // Apply only on non-keysounded files..
		TimeCompensation += Configuration::GetConfigf("Offset7K");

	double Drift = TimeCompensation;

	Log::Logf("TimeCompensation: %f (Latency: %f / Offset: %f)\n", TimeCompensation, MixerGetLatency(), CurrentDiff->Offset);


	/*
 * 		There are three kinds of speed modifiers:
 * 			-CMod (Keep speed the same through the song, equal to a constant)
 * 			-MMod (Find highest speed and set multiplier to such that the highest speed is equal to a constant)
 *			-First (Find the first speed in the chart, and set multiplier to such that the first speed is equal to a constant)
 *
 *			The calculations are done ahead, and while SpeedConstant = 0 either MMod or first are assumed
 *			but only if there's a constant specified by the user.
 * */

	// What, you mean we don't have timing data at all?
	if (CurrentDiff->Timing.size() == 0)
	{
		Log::Printf("Error loading chart: No timing data.\n");
		return false;
	}

	Log::Printf("Processing song... ");

	if (DesiredDefaultSpeed)
	{
		if (Type == SPEEDTYPE_CMOD) // cmod
		{
			SpeedMultiplierUser = 1;
			SpeedConstant = DesiredDefaultSpeed;
		}

		CurrentDiff->Process(NotesByChannel, BPS, VSpeeds, Drift, SpeedConstant);

		if (Type == SPEEDTYPE_MMOD) // mmod
		{
			double max = 0; // Find the highest speed
			for (TimingData::iterator i = VSpeeds.begin();
				i != VSpeeds.end();
				i++)
			{
				max = std::max(max, abs(i->Value));
			}

			double Ratio = DesiredDefaultSpeed / max; // How much above or below are we from the maximum speed?
			SpeedMultiplierUser = Ratio;
		}else if (Type == SPEEDTYPE_FIRST) // We use this case as default. The logic is "Not a CMod, Not a MMod, then use first, the default.
		{
			double DesiredMultiplier =  DesiredDefaultSpeed / VSpeeds[0].Value;

			SpeedMultiplierUser = DesiredMultiplier;
		}else if (Type != SPEEDTYPE_CMOD) // other cases
		{
			double bpsd = 4.0/(BPS[0].Value);
			double Speed = (MeasureBaseSpacing / bpsd);
			double DesiredMultiplier = DesiredDefaultSpeed / Speed;

			SpeedMultiplierUser = DesiredMultiplier;
		}

	}else
		CurrentDiff->Process(NotesByChannel, BPS, VSpeeds, Drift); // Regular processing
	return true;
}

bool ScreenGameplay7K::LoadBMPs()
{
	if (Configuration::GetConfigf("DisableBMP") == 0 && CurrentDiff->Data->BMPEvents)
	{
		if (CurrentDiff->Data->BMPEvents->BMPList.size())
			Log::Printf("Loading BMPs...\n");

		for (std::map<int, GString>::iterator i = CurrentDiff->Data->BMPEvents->BMPList.begin();
			i != CurrentDiff->Data->BMPEvents->BMPList.end(); i++)
			BMPs.AddToListIndex(i->second, MySong->SongDirectory, i->first);

		// We don't need this any more.
		CurrentDiff->Data->BMPEvents->BMPList.clear();

		BMPEvents = CurrentDiff->Data->BMPEvents->BMPEventsLayerBase;
		BMPEventsMiss = CurrentDiff->Data->BMPEvents->BMPEventsLayerMiss;
		BMPEventsLayer = CurrentDiff->Data->BMPEvents->BMPEventsLayer;
		BMPEventsLayer2 = CurrentDiff->Data->BMPEvents->BMPEventsLayer2;
	}

	return true;
}

void ScreenGameplay7K::SetupAfterLoadingVariables()
{
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
	CurrentBeat = IntegrateToTime(BPS, 0);

	RecalculateMatrix();
	MultiplierChanged = true;

	BarlineEnabled = (Configuration::GetSkinConfigf("BarlineEnabled") != 0);
	BarlineOffsetKind = Configuration::GetSkinConfigf("BarlineOffset");
	BarlineX = Configuration::GetSkinConfigf("BarlineX");
	BarlineWidth = Configuration::GetSkinConfigf("BarlineWidth");

	if (BarlineEnabled)
	{
		CurrentDiff->GetMeasureLines(MeasureBarlines, VSpeeds, WaitingTime);

		int UpscrollMod = Upscroll ? -1 : 1;
		BarlineOffset = BarlineOffsetKind == 0 ? NoteHeight * UpscrollMod / 2 : 0;
	}

	ErrorTolerance = Configuration::GetConfigf("ErrorTolerance");

	if (ErrorTolerance <= 0)
		ErrorTolerance = 5; // ms

}

void ScreenGameplay7K::ChangeNoteTimeToBeats()
{
	for (uint32 k = 0; k < CurrentDiff->Channels; k++)
	{
		for (std::vector<VSRG::TrackNote>::iterator m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); m++)
		{
			double beatStart = IntegrateToTime(BPS, m->GetDataStartTime());
			double beatEnd = IntegrateToTime(BPS, m->GetDataEndTime());
			m->GetDataStartTime() = beatStart;
			m->GetDataEndTime() = beatEnd;
		}
	}
}

void ScreenGameplay7K::SetupMechanics()
{
	
	// This must be done before setLifeTotal in order for it to work.
	score_keeper->setMaxNotes(CurrentDiff->TotalScoringObjects);
	
	if (Configuration::GetConfigf("AlwaysUseRaindropMechanics") == 0 && CurrentDiff->Data->TimingInfo)
	{
		VSRG::CustomTimingInfo * TimingInfo = CurrentDiff->Data->TimingInfo;
		if (TimingInfo->GetType() == VSRG::TI_BMS)
		{
			VSRG::BmsTimingInfo *Info = static_cast<VSRG::BmsTimingInfo*> (TimingInfo);
			score_keeper->setLifeTotal(Info->life_total);
			score_keeper->setJudgeRank(Info->judge_rank);
			UsedTimingType = TT_TIME;
			lifebar_type = LT_GROOVE;
		}
		else if (TimingInfo->GetType() == VSRG::TI_OSUMANIA)
		{
			VSRG::OsuManiaTimingInfo *Info = static_cast<VSRG::OsuManiaTimingInfo*> (TimingInfo);
			score_keeper->setODWindows(Info->OD);
			lifebar_type = LT_STEPMANIA;
			scoring_type = ST_OSUMANIA;
			UsedTimingType = TT_TIME;
		}
		else if (TimingInfo->GetType() == VSRG::TI_O2JAM)
		{
			VSRG::O2JamTimingInfo *O2Info = (VSRG::O2JamTimingInfo*) TimingInfo;
			lifebar_type = LT_O2JAM;
			UsedTimingType = TT_BEATS;
			scoring_type = ST_O2JAM;
			score_keeper->setJudgeRank(-100); // Special constant to notify beat based timing.
			score_keeper->setO2LifebarRating(O2Info->Difficulty);
		}
		else if (TimingInfo->GetType() == VSRG::TI_STEPMANIA)
		{
			// lifebar_type = LT_STEPMANIA;
			lifebar_type = LT_GROOVE;
			UsedTimingType = TT_TIME;
			score_keeper->setLifeTotal(-1);
			score_keeper->setJudgeRank(Configuration::GetConfigf("DefaultJudgeRank"));
		}
		else
		{
			lifebar_type = LT_GROOVE;
			UsedTimingType = TT_TIME;
			score_keeper->setLifeTotal(-1);
			score_keeper->setJudgeRank(Configuration::GetConfigf("DefaultJudgeRank"));
		}
	}
	else
	{
		lifebar_type = LT_GROOVE;
		score_keeper->setLifeTotal(-1);
		score_keeper->setJudgeRank(Configuration::GetConfigf("DefaultJudgeRank"));
		UsedTimingType = TT_TIME;
		lifebar_type = LT_GROOVE;
	}
	
	if(Configuration::GetConfigf("AlwaysUseRidiculousTiming")){
		score_keeper->set_manual_w0(true);
	}

	/*
		If we're on TT_BEATS we've got to recalculate all note positions to beats, 
		and use mechanics that use TT_BEATS as its timing type.
	*/

	if (UsedTimingType == TT_TIME)
	{
		Log::Printf("Using raindrop mechanics set!\n");
		MechanicsSet = new RaindropMechanics;
	}
	else if (UsedTimingType == TT_BEATS)
	{
		Log::Printf("Using o2jam mechanics set!\n");
		MechanicsSet = new O2JamMechanics;
		ChangeNoteTimeToBeats();
	}

	MechanicsSet->Setup(MySong, CurrentDiff, score_keeper);
	MechanicsSet->HitNotify = bind(&ScreenGameplay7K::HitNote, this, _1, _2, _3, _4);
	MechanicsSet->MissNotify = bind(&ScreenGameplay7K::MissNote, this, _1, _2, _3, _4, _5);
	MechanicsSet->IsLaneKeyDown = bind(&ScreenGameplay7K::GetGearLaneState, this, _1);
	MechanicsSet->SetLaneHoldingState = bind(&ScreenGameplay7K::SetLaneHoldState, this, _1, _2);
	MechanicsSet->PlayLaneSoundEvent = bind(&ScreenGameplay7K::PlayLaneKeysound, this, _1);
	MechanicsSet->PlayNoteSoundEvent = bind(&ScreenGameplay7K::PlayKeysound, this, _1);
}

void ScreenGameplay7K::LoadThreadInitialization()
{
	GString MissSndFile = (GameState::GetInstance().GetSkinFile("miss.ogg"));
	GString FailSndFile = (GameState::GetInstance().GetSkinFile("stage_failed.ogg"));

	if (Utility::FileExists(MissSndFile))
	{
		MissSnd = new SoundSample();
		if (MissSnd->Open(MissSndFile.c_str()))
			MixerAddSample(MissSnd);
		else
			delete MissSnd;
	}

	if (Utility::FileExists(FailSndFile))
	{
		FailSnd = new SoundSample();
		if (FailSnd->Open(FailSndFile.c_str()))
			MixerAddSample(FailSnd);
		else
			delete FailSnd;
	}

	if (!LoadChartData() || !LoadSongAudio() || !ProcessSong() || !LoadBMPs())
	{
		DoPlay = false;
		return;
	}

	SetupMechanics();

	SetupAfterLoadingVariables();
	SetupLua();
	Log::Printf("Done.\n");

	DoPlay = true;

	AssignMeasure(StartMeasure);

	// We're done with the data stored in the difficulties that aren't the one we're using. Clear it up.
	for (auto i = MySong->Difficulties.begin(); i != MySong->Difficulties.end(); i++)
	{
		(*i)->Destroy();
	}
}

void ScreenGameplay7K::SetupGear()
{
	using namespace Configuration;
	char str[256];
	char cstr[256];
	char nstr[256];

	if (!GameState::GetInstance().SkinSupportsChannelCount(CurrentDiff->Channels))
	{
		Log::Printf("Unsupported skin key count: %d", CurrentDiff->Channels);
		DoPlay = false;
		return;
	}

	sprintf(nstr, "Channels%d", CurrentDiff->Channels);

	for (int i = 0; i < CurrentDiff->Channels; i++)
	{
		sprintf(cstr, "Key%d", i + 1);
		GearLaneImage[i] = GameState::GetInstance().GetSkinImage(GetSkinConfigs(cstr, nstr));

		sprintf(cstr, "Key%dDown", i + 1);
		GearLaneImageDown[i] = GameState::GetInstance().GetSkinImage(GetSkinConfigs(cstr, nstr));

		sprintf(str, "Key%dX", i+1);
		LanePositions[i] = Configuration::GetSkinConfigf(str, nstr);

		sprintf(str, "Key%dWidth", i+1);
		LaneWidth[i] = Configuration::GetSkinConfigf(str, nstr);

		float ChanBarlineWidth;
		sprintf(str, "BarlineWidth");
		ChanBarlineWidth = Configuration::GetSkinConfigf(str, nstr);
		if (ChanBarlineWidth)
			BarlineWidth = ChanBarlineWidth;

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

void ScreenGameplay7K::SetupBackground()
{
	Image* BackgroundImage = ImageLoader::Load(MySong->SongDirectory + MySong->BackgroundFilename);

	if (BackgroundImage)
		Background.SetImage(BackgroundImage);
	else
	{
		// Caveat 2: Try to automatically load background.
		std::vector<GString> DirCnt;
		Directory SngDir = MySong->SongDirectory;

		SngDir.ListDirectory(DirCnt, Directory::FS_REG);
		for (std::vector<GString>::iterator i = DirCnt.begin();
			i != DirCnt.end();
			i++)
		{
			GString ext = Directory(*i).GetExtension();
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
}


void ScreenGameplay7K::MainThreadInitialization()
{
	SetupGear();

	if (!DoPlay) // Failure to load something important?
	{
		Running = false;
		return;
	}

	PlayReactiveSounds = (CurrentDiff->IsVirtual || !(Configuration::GetConfigf("DisableHitsounds")));
	MsDisplayMargin = (Configuration::GetSkinConfigf("HitErrorDisplayLimiter"));

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

		GString Filename = Configuration::GetSkinConfigs(cstr, nstr);
		NoteImages[i] = GameState::GetInstance().GetSkinImage(Filename);

		/* Hold head image */
		sprintf(cstr, "Key%dHoldHeadImage", i+1);

		Filename = Configuration::GetSkinConfigs(cstr, nstr);
		NoteImagesHoldHead[i] = GameState::GetInstance().GetSkinImage(Filename);
		if (!NoteImagesHoldHead[i])
			NoteImagesHoldHead[i] = NoteImages[i];

		/* Hold tail image */
		sprintf(cstr, "Key%dHoldTailImage", i+1);

		Filename = Configuration::GetSkinConfigs(cstr, nstr);
		NoteImagesHoldTail[i] = GameState::GetInstance().GetSkinImage(Filename);
		if (!NoteImagesHoldTail[i])
			NoteImagesHoldTail[i] = NoteImages[i];

		/* Hold body image */
		sprintf(cstr, "Key%dHoldImage", i+1);

		Filename = Configuration::GetSkinConfigs(cstr, nstr);
		NoteImagesHold[i] = GameState::GetInstance().GetSkinImage(Filename);

		lastClosest[i] = 0;

		HeldKey[i] = NULL;
	}

	NoteImage = GameState::GetInstance().GetSkinImage("note.png");
	SetupBackground();

	WindowFrame.SetLightMultiplier(0.75f);

	Animations->Initialize("", false);

	memset(PlaySounds, 0, sizeof(PlaySounds));

	CalculateHiddenConstants();

	if (!StartMeasure)
		WaitingTime = abs(std::min(-WaitingTime, CurrentDiff->Offset - 1.5));
	else
		WaitingTime = 0;


	CurrentBeat = IntegrateToTime(BPS, -WaitingTime);
	Animations->GetImageList()->ForceFetch();

	BMPs.LoadAll();
// 	BMPs.ForceFetch();
	Running = true;
}
