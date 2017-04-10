#include "pch.h"

#include "Screen.h"
#include "Sprite.h"
#include "ScreenVideoTest.h"
#include "Texture.h"
#include "VideoPlayback.h"

VideoPlayback play(2);

ScreenVideoTest::ScreenVideoTest() : Screen("ScreenVideoTest", nullptr)
{
	clock = 0;
	play.Open("bga.mp4");

	play.StartDecodeThread();

	Running = true;
	sprite.SetImage(&play);
}

bool ScreenVideoTest::Run(double dt)
{
	play.UpdateClock(clock);

	sprite.Render();
	
	clock += dt;
	return true;
}


