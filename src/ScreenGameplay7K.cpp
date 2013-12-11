#include "Global.h"
#include "Screen.h"
#include "Audio.h"

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

void ScreenGameplay7K::Init(Song7K* S)
{
	MySong = S;
}

void ScreenGameplay7K::LoadThreadInitialization()
{
}

void ScreenGameplay7K::MainThreadInitialization()
{
}

void ScreenGameplay7K::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
}


bool ScreenGameplay7K::Run(double Delta)
{
	float SongDelta;

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

	PositionMatrix = glm::translate(PositionMatrix, glm::vec3(0, Speed * SongDelta, 0));
	
	int32 r;
	Music->GetStream()->UpdateBuffer(r);
	return Running;
}