#include "Global.h"
#include "Screen.h"
#include "Audio.h"
#include "FileManager.h"
#include "ImageLoader.h"

#include "Song.h"
#include "ScreenGameplay7K.h"
#include "BitmapFont.h"

BitmapFont * GFont = NULL;

ScreenGameplay7K::ScreenGameplay7K()
{
	Measure = 0;
	Speed = 0; // Recalculate.
	SpeedMultiplier = 1;
	SongOldTime = -1;
	Music = NULL;
	deltaPos = 0;

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

void ScreenGameplay7K::RecalculateMatrix()
{
	PositionMatrix = glm::scale(glm::translate(glm::mat4(), 
			glm::vec3(GearLaneWidth/2 + GearStartX, BasePos + CurrentVertical * SpeedMultiplier + deltaPos, 0)), 
			glm::vec3(GearLaneWidth, 10, 0));
}

void ScreenGameplay7K::LoadThreadInitialization()
{
	char* SkinFiles [] =
	{
		"key1.png",
		"key2.png",
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

		/* Apply speed multiplier. fixme: this is overly obtuse. there MUST be a better way :v */
		/*for (uint32 i = 0; i < NotesByMeasure[k].size(); i++)
		{
			for (uint32 q = 0; q < NotesByMeasure[k][i].MeasureNotes.size(); q++)
			{
				NotesByMeasure[k][i].MeasureNotes[q].AssignSpeedMultiplier(SpeedMultiplier);
			}
		}*/
	}


	/* Initial object distance */
	float VertDistance = ((CurrentDiff->Offset / spb(CurrentDiff->Timing[0].Value)) / MySong->MeasureLength) * MeasureBaseSpacing;
	BasePos = float(ScreenHeight) - GearHeight;
	CurrentVertical = -(VertDistance);

	RecalculateMatrix();
}

void ScreenGameplay7K::MainThreadInitialization()
{
	for (int i = 0; i < CurrentDiff->Channels; i++)
	{
		Keys[i].SetImage ( i % 2 ? ImageLoader::LoadSkin("key2.png") : ImageLoader::LoadSkin("key1.png") );
		Keys[i].SetSize( GearWidth / CurrentDiff->Channels, GearHeight );
		Keys[i].Centered = true;
		Keys[i].SetPosition( GearStartX + GearLaneWidth * i + GearLaneWidth / 2, ScreenHeight - GearHeight/2 );
	}

	NoteImage = ImageLoader::LoadSkin("note.png");

	Background.SetImage(ImageLoader::Load(MySong->BackgroundDir));
	Background.AffectedByLightning = true;
	Running = true;
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
			SpeedMultiplier += 0.25;
			break;
		case KT_FractionDec:
			SpeedMultiplier -= 0.25;
			break;
		case KT_Left:
			deltaPos -= 10;
			break;
		case KT_Right:
			deltaPos += 10;
			break;
		default:
			break;
		}
	}
}


bool ScreenGameplay7K::Run(double Delta)
{
	float SongDelta, SongTime;

	ScreenTime += Delta;

	if (ScreenTime > 3)
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
		RecalculateMatrix();


		/* Update music. */
		int32 r;
		Music->GetStream()->UpdateBuffer(r);
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
#endif

	GFont->DisplayText(ss.str().c_str(), glm::vec2(0,0));
	return Running;
}