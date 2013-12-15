#include "Global.h"
#include "Screen.h"
#include "Audio.h"
#include "FileManager.h"
#include "ImageLoader.h"

#include "Song.h"
#include "ScreenGameplay7K.h"


ScreenGameplay7K::ScreenGameplay7K()
{
	Measure = 0;
	Speed = 0; // Recalculate.
	SpeedMultiplier = 1;
	SongOldTime = -1;

	/* Start position at judgement line.*/
	PositionMatrix = glm::translate(glm::mat4(), glm::vec3(0, ScreenHeight - GearHeight /* - JudgelineHeight/2 */, 0));
}

void ScreenGameplay7K::Cleanup()
{
}

void ScreenGameplay7K::Init(Song7K* S, int DifficultyIndex)
{
	MySong = S;
	CurrentDiff = S->Difficulties[DifficultyIndex];
}

void ScreenGameplay7K::LoadThreadInitialization()
{
	char* SkinFiles [] =
	{
		"key1.png",
		"key2.png"
	};

	ImageLoader::LoadFromManifest(SkinFiles, 3, FileManager::GetSkinPrefix());
	
	char* OtherFiles [] =
	{
		(char*)MySong->BackgroundDir.c_str()
	};

	ImageLoader::LoadFromManifest(OtherFiles, 1);
	/* TODO: Add playfield background */


	for (int i = 0; i < CurrentDiff->Channels; i++)
	{
		/* copy all size() measures of key k into notes by measure of key k */
		for (int k = 0; k < CurrentDiff->Measures[i].size(); k++)
			NotesByMeasure[i].push_back(CurrentDiff->Measures[i].at(k));
	}
}

void ScreenGameplay7K::MainThreadInitialization()
{
	for (int i = 0; i < CurrentDiff->Channels; i++)
	{
		Keys[i].SetImage ( i % 2 ? ImageLoader::LoadSkin("key2.png") : ImageLoader::LoadSkin("key1.png") );
		Keys[i].SetSize( GearWidth / CurrentDiff->Channels, GearHeight );
		Keys[i].Centered = true;
		Keys[i].SetPosition( (GearWidth / 2) / CurrentDiff->Channels * i, ScreenHeight - GearHeight/2 );
	}

	
	Background.SetImage(ImageLoader::Load(MySong->BackgroundDir));
	Background.AffectedByLightning = true;
}

void ScreenGameplay7K::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	/* 
	 In here we should use the input arrangements depending on 
	 the amount of channels the current difficulty is using.
	 Also potentially pausing and quitting the screen.
	 Other than that most input can be safely ignored.
	*/
}


bool ScreenGameplay7K::Run(double Delta)
{
	float SongDelta, SongTime;

	ScreenTime += Delta;

	if (ScreenTime < 2)
		return Running;

	if (!Music)
		return false; // Quit inmediately. There's no point.

	if (SongOldTime == -1)
	{
		Music->Start(false, false);
		SongOldTime = 0;
	}

	SongDelta = Music->GetPlaybackTime() - SongOldTime;
	SongTime = Music->GetPlaybackTime();

	/* Update velocity. */
	if (VSpeeds.size() && SongTime >= VSpeeds.at(0).Time)
	{
		Speed = VSpeeds.at(0).Value;
		VSpeeds.erase(VSpeeds.begin());
	}

	PositionMatrix = glm::translate(PositionMatrix, glm::vec3(0, Speed * SongDelta * SpeedMultiplier, 0));
	
	Background.Render();

	for (int32 i = 0; i < CurrentDiff->Channels; i++)
		Keys[i].Render();

	/* Update music. */
	int32 r;
	Music->GetStream()->UpdateBuffer(r);
	return Running;
}