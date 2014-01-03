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


#include "ScreenGameplay7K.h"

BitmapFont * GFont = NULL;
int lastPressed = 0;
int lastMsOff[MAX_CHANNELS];
int lastClosest[MAX_CHANNELS];

/* Time before actually starting everything. */
#define WAITING_TIME 5
#define MS_CUTOFF 100
#define EQ(x) (-100.0/84.0)*x + 10000.0/84.0


ScreenGameplay7K::ScreenGameplay7K()
{
	Speed = 0; // Recalculate.
	SpeedMultiplier = 0;
	SongOldTime = -1;
	Music = NULL;
	deltaPos = 0;

	waveEffectEnabled = false;
	waveEffect = 0;

	SpeedMultiplierUser = 1;

	CurrentVertical = 0;
	SongTime = 0;

	if (!GFont)
	{	
		GFont = new BitmapFont();
		GFont->LoadSkinFontImage("font.tga", glm::vec2(18, 32), glm::vec2(34,34), glm::vec2(10,16), 32);
	}
}

void ScreenGameplay7K::Cleanup()
{
	if (Music)
		Music->Stop();
}

void ScreenGameplay7K::Init(Song7K* S, int DifficultyIndex, bool UseUpscroll)
{
	MySong = S;
	CurrentDiff = S->Difficulties[DifficultyIndex];
	Upscroll = UseUpscroll;
}

void ScreenGameplay7K::RecalculateEffects()
{
	float SongTime = 0;
	
	if (Music)
		SongTime = Music->GetPlaybackTime();

	if (waveEffectEnabled)
	{
		waveEffect = sin(SongTime) * 0.5 * SpeedMultiplierUser;
	}
}

void ScreenGameplay7K::RunMeasures()
{
	typedef std::vector<SongInternal::Measure<TrackNote>> NoteVector;

	for (unsigned int k = 0; k < Channels; k++)
	{
		NoteVector &Measures = NotesByMeasure[k];

		for (NoteVector::iterator i = Measures.begin(); i != Measures.end(); i++)
		{
			for (std::vector<TrackNote>::iterator m = (*i).MeasureNotes.begin(); m != (*i).MeasureNotes.end(); m++)
			{
				/* We have to check for all gameplay conditions for this note. */

				if ((SongTime - m->GetStartTime()) * 1000 > MS_CUTOFF)
				{
					Score.TotalNotes++;

					/* remove note from judgement*/
					 m = (*i).MeasureNotes.erase(m);

					if (m == (*i).MeasureNotes.end())
						goto next_measure;
				}
			}
			next_measure:;
		}
	}

}

void ScreenGameplay7K::JudgeLane(unsigned int Lane)
{
	typedef std::vector<SongInternal::Measure<TrackNote>> NoteVector;
	NoteVector &Measures = NotesByMeasure[Lane];

	lastPressed = Lane;

	if (!Music)
		return;

	lastClosest[Lane] = 1000000000;

	for (NoteVector::iterator i = Measures.begin(); i != Measures.end(); i++)
	{
		for (std::vector<TrackNote>::iterator m = (*i).MeasureNotes.begin(); m != (*i).MeasureNotes.end(); m++)
		{
			double tD = abs (m->GetStartTime() - SongTime) * 1000;
			// std::cout << "\n time: " << m->GetStartTime() << " st: " << SongTime << " td: " << tD;

			lastClosest[Lane] = std::min(tD, (double)lastClosest[Lane]);

			if (tD > MS_CUTOFF)
				goto next_note;
			else
			{
				/* first iteration of the accuracy equation */
				float accPercent = EQ(tD);
				
				if (accPercent > 100)
					accPercent = 100;

				lastMsOff[Lane] = tD;

				Score.Accuracy += accPercent;
				Score.TotalNotes++;

				ExplosionTime[Lane] = 0;

				/* remove note from judgement*/
				(*i).MeasureNotes.erase(m);

				return; // we judged a note in this lane, so we're done.
			}
			next_note: ;
		}
	}
}

void ScreenGameplay7K::RecalculateMatrix()
{
	if (Upscroll)
		SpeedMultiplier = - (SpeedMultiplierUser + waveEffect);
	else
		SpeedMultiplier = SpeedMultiplierUser + waveEffect;

	PositionMatrix = glm::scale(glm::translate(glm::mat4(), 
			glm::vec3(GearLaneWidth/2 + GearStartX, BasePos + CurrentVertical * SpeedMultiplier + deltaPos, 0)), 
			glm::vec3(GearLaneWidth, 10, 0));
}

void ScreenGameplay7K::LoadThreadInitialization()
{
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
		"note.png"
	};

	ImageLoader::LoadFromManifest(SkinFiles, 3, FileManager::GetSkinPrefix());
	
	char* OtherFiles [] =
	{
		(char*)MySong->BackgroundDir.c_str()
	};

	ImageLoader::LoadFromManifest(OtherFiles, 1);
	/* TODO: Add playfield background */

	GearLaneWidth = GearWidth / CurrentDiff->Channels;

	if (!Music)
	{
		Music = new PaStreamWrapper(MySong->SongFilename.c_str());
	}

	Channels = CurrentDiff->Channels;
	VSpeeds = CurrentDiff->VerticalSpeeds;

	/* Set up notes. */
	for (uint32 k = 0; k < Channels; k++)
	{
		/* Copy measures. (Eek!) */
		for (std::vector<SongInternal::Measure<TrackNote>>::iterator i = CurrentDiff->Measures[k].begin(); 
			i != CurrentDiff->Measures[k].end();
			i++)
		{
			NotesByMeasure[k].push_back(*i);
		}
	}


	/* Initial object distance */
	if (!Upscroll)
		JudgementLinePos = float(ScreenHeight) - GearHeight;
	else
		JudgementLinePos = GearHeight;

	BasePos = JudgementLinePos + (Upscroll ? 5 : -5) /* NoteSize/2 ;P */;
	CurrentVertical -= VSpeeds.at(0).Value * (WAITING_TIME + CurrentDiff->Offset - GetDeviceLatency());
	Speed = VSpeeds.at(0).Value; /* First speed is always at offset time, so we have to wait that much longer */
	RecalculateMatrix();
}

void ScreenGameplay7K::MainThreadInitialization()
{
	for (int i = 0; i < CurrentDiff->Channels; i++)
	{
		std::stringstream ss;
		char str[256];
		char cstr[256];
		char nstr[256];

		/* eventually move this to its own space (this is unlikely to overflow by the way) */
		sprintf(cstr, "Key%d", i+1);
		sprintf(nstr, "Channels%d", CurrentDiff->Channels);
		
		/* If it says that the nth lane uses the kth key then we'll bind that! */
		sprintf(str, "key%d.png", (int)Configuration::GetSkinConfigf(cstr, nstr));
		GearLaneImage[i] = ImageLoader::LoadSkin(str);

		str[0] = 0;
		sprintf(str, "key%dd.png", (int)Configuration::GetSkinConfigf(cstr, nstr));
		GearLaneImageDown[i] = ImageLoader::LoadSkin(str);

		/* Assign per-lane bindings. */
		sprintf(cstr, "Key%dBinding", i+1);

		int Binding = Configuration::GetSkinConfigf(cstr, nstr);
		GearBindings[Binding - 1] = i;

		sprintf(cstr, "Key%dImage", i+1);

		std::string Filename = Configuration::GetSkinConfigs(cstr, nstr);
		NoteImages[i] = ImageLoader::LoadSkin(Filename);

		Keys[i].SetImage ( GearLaneImage[i] );
		Keys[i].SetSize( GearLaneWidth, GearHeight );
		Keys[i].Centered = true;

		Keys[i].SetPosition( GearStartX + GearLaneWidth * i + GearLaneWidth / 2, JudgementLinePos + (Upscroll? -1:1) * GearHeight/2 );

		if (Upscroll)
			Keys[i].SetRotation(180);

		Explosion[i].Centered = true;
		Explosion[i].SetSize( GearLaneWidth * 2, GearLaneWidth * 2 );
		Explosion[i].SetPosition( GearStartX + GearLaneWidth * i + GearLaneWidth / 2, JudgementLinePos );
		lastClosest[i] = 0;
	}

	NoteImage = ImageLoader::LoadSkin("note.png");

	Background.SetImage(ImageLoader::Load(MySong->BackgroundDir));
	Background.AffectedByLightning = true;

	if (Background.GetImage())
	{
		float SizeRatio = 768 / Background.GetHeight();
		Background.SetScale(SizeRatio);
		Background.Centered = true;
		Background.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	}

	for (int i = 0; i < 20 /*Frames*/; i++)
	{
		char str[256];
		sprintf(str, "Explosion-%d.png", i);
		ExplosionFrames[i] = ImageLoader::LoadSkin(str);
	}

	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		ExplosionTime[i] = 0.016 * 20;
	}

	WindowFrame.SetLightMultiplier(0.6f);
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

	if (KeyDown)
	{
		JudgeLane(GearIndex);
		Keys[GearIndex].SetImage( GearLaneImageDown[GearIndex], false );
	}
	else
		Keys[GearIndex].SetImage( GearLaneImage[GearIndex], false );
}

void ScreenGameplay7K::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	/* 
	 In here we should use the input arrangements depending on 
	 the amount of channels the current difficulty is using.
	 Also potentially pausing and quitting the screen.
	 Other than that most input can be safely ignored.
	*/

	if (code == KE_Press)
	{
		switch (BindingsManager::TranslateKey(key))
		{
		case KT_Escape:
			Running = false;
			break;
		case KT_FractionInc:
			SpeedMultiplierUser += 0.25;
			break;
		case KT_FractionDec:
			SpeedMultiplierUser -= 0.25;
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


bool ScreenGameplay7K::Run(double Delta)
{
	float SongDelta;

	ScreenTime += Delta;

	for (int i = 0; i < CurrentDiff->Channels; i++)
	{
		ExplosionTime[i] += Delta;
	}

	if (ScreenTime > WAITING_TIME)
	{

		if (!Music || !Music->GetStream())
			return false; // Quit inmediately. There's no point.

		if (SongOldTime == -1)
		{
			Music->Start(false, false);
			SongOldTime = 0;
		}

		SongDelta = Music->GetPlaybackTime() - SongOldTime;
		SongTime += SongDelta;

		UpdateVertical();
		RunMeasures();
		RecalculateEffects();
		RecalculateMatrix();

		SongOldTime = SongTime;

		/* Update music. */
		int32 r;
		Music->GetStream()->UpdateBuffer(r);
	}else
	{
		Speed = VSpeeds.at(0).Value;
		CurrentVertical += Speed * Delta; 
		RecalculateMatrix();
	}

	Background.Render();

	DrawMeasures();

	for (int32 i = 0; i < CurrentDiff->Channels; i++)
		Keys[i].Render();

	DrawExplosions();

	std::stringstream ss;

	ss << "score: " << Score.Accuracy;
	ss << "\naccuracy: " << Score.Accuracy / (Score.TotalNotes ? Score.TotalNotes : 1);
	ss << "\nMult/Speed: " << SpeedMultiplier << "x / " << SpeedMultiplier*4 << "\n";

	GFont->DisplayText(ss.str().c_str(), glm::vec2(0,0));

	for (unsigned int i = 0; i < Channels; i++)
	{
		std::stringstream ss;
		ss << lastClosest[i];
		GFont->DisplayText(ss.str().c_str(), Keys[i].GetPosition());
	}

	return Running;
}

void ScreenGameplay7K::UpdateVertical()
{
	/* Update velocity. Use proper integration. */
	double SongDelta = SongTime - SongOldTime;
	uint32 Idx = SectionIndex(VSpeeds, SongOldTime) - 1;
	TimingData IntervalTiming;

	GetTimingChangesInInterval(VSpeeds, SongOldTime, SongTime, IntervalTiming);
	
	if (IntervalTiming.size())
	{
		SongInternal::TimingSegment Current = VSpeeds[Idx];
		double OldTime = SongOldTime;
		
		uint32 size = IntervalTiming.size();

		for (uint32 i = 0; i < size; i++)
		{
			double Change = (IntervalTiming[i].Time - OldTime) * Current.Value;
			CurrentVertical += Change;
			Current = IntervalTiming[i];
			OldTime = Current.Time;
		}

		/* And then finish. */
		CurrentVertical += (SongTime - Current.Time) * Current.Value;
	}
	else
	{
		CurrentVertical += VSpeeds[Idx].Value * SongDelta;
	}
}