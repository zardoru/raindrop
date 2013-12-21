#include <stdio.h>

#include "Global.h"
#include "Screen.h"
#include "Configuration.h"
#include "Audio.h"
#include "FileManager.h"
#include "ImageLoader.h"
#include "Song.h"
#include "ScreenGameplay7K.h"
#include "BitmapFont.h"

BitmapFont * GFont = NULL;

/* Time before actually starting everything. */
#define WAITING_TIME 5

ScreenGameplay7K::ScreenGameplay7K()
{
	Measure = 0;
	Speed = 0; // Recalculate.
	SpeedMultiplier = 0;
	SongOldTime = -1;
	Music = NULL;
	deltaPos = 0;

	waveEffectEnabled = false;
	waveEffect = 0;

	SpeedMultiplierUser = 1;

	CurrentVertical = 0;

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

void ScreenGameplay7K::Init(Song7K* S, int DifficultyIndex)
{
	MySong = S;
	CurrentDiff = S->Difficulties[DifficultyIndex];
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

void ScreenGameplay7K::RecalculateMatrix()
{
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
	float VertDistance = ((CurrentDiff->Offset / spb(CurrentDiff->Timing[0].Value)) / MySong->MeasureLength) * MeasureBaseSpacing;
	BasePos = float(ScreenHeight) - GearHeight;
	// CurrentVertical = -(VertDistance);
	CurrentVertical -= VSpeeds.at(0).Value * (WAITING_TIME + CurrentDiff->Offset);

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

		Keys[i].SetImage ( GearLaneImage[i] );
		Keys[i].SetSize( GearLaneWidth, GearHeight );
		Keys[i].Centered = true;
		Keys[i].SetPosition( GearStartX + GearLaneWidth * i + GearLaneWidth / 2, ScreenHeight - GearHeight/2 );
	}

	NoteImage = ImageLoader::LoadSkin("note.png");

	Background.SetImage(ImageLoader::Load(MySong->BackgroundDir));
	Background.AffectedByLightning = true;
	Running = true;
}

void ScreenGameplay7K::TranslateKey(KeyType K, bool KeyDown)
{
	int Index = K - KT_Key1; /* Bound key */
	int GearIndex = GearBindings[Index]; /* Binding this key to a lane */

	if (Index > 13 || Index < 0)
		return;

	if (GearIndex > 13 || GearIndex < 0)
		return;

	if (KeyDown)
		Keys[GearIndex].SetImage( GearLaneImageDown[GearIndex] );
	else
		Keys[GearIndex].SetImage( GearLaneImage[GearIndex] );
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
		default:
			if (BindingsManager::TranslateKey(key) >= KT_Key1)
				TranslateKey(BindingsManager::TranslateKey(key), true);
			break;
		}
	}else
	{
		switch (BindingsManager::TranslateKey(key))
		{
		default:
			if (BindingsManager::TranslateKey(key) >= KT_Key1)
				TranslateKey(BindingsManager::TranslateKey(key), false);
			break;
		}
	}
}


bool ScreenGameplay7K::Run(double Delta)
{
	float SongDelta, SongTime = 0;

	ScreenTime += Delta;

	if (ScreenTime > WAITING_TIME)
	{

		if (!Music)
			return false; // Quit inmediately. There's no point.

		if (SongOldTime == -1)
		{
			Music->Start(false, false);
			SongOldTime = 0;
		}

		SongDelta = Music->GetPlaybackTime() - SongOldTime;
		SongTime = Music->GetPlaybackTime();
		SongOldTime = SongTime;

		/* Update velocity. */
		if (VSpeeds.size() && SongTime >= VSpeeds.at(0).Time)
		{
			Speed = VSpeeds.at(0).Value;
			VSpeeds.erase(VSpeeds.begin());
		}

		
		CurrentVertical += Speed * SongDelta;
		RecalculateEffects();
		RecalculateMatrix();


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

#ifndef NDEBUG
	std::stringstream ss;
	ss << "cver: " << CurrentVertical;
	ss << "\ncverm: " << CurrentVertical * SpeedMultiplier;
	ss << "\nsm: "    << SpeedMultiplier;
	ss << "\ndpos: " << deltaPos;
	ss << "\nrat: " << (CurrentVertical + deltaPos) / CurrentVertical;
	ss << "\nst: " << SongTime;
	GFont->DisplayText(ss.str().c_str(), glm::vec2(0,0));
#endif
	return Running;
}