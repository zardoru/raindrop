#include "Global.h"
#include "GraphObject2D.h"
#include "ScreenLoading.h"
#include "ImageLoader.h"

void LoadFunction(void* Screen)
{
	IScreen *S = (IScreen*)Screen;
	S->Init();
}

ScreenLoading::ScreenLoading(IScreen *Parent, IScreen *_Next)
{
	Next = _Next;
	LoadThread = NULL;
	Running = true;
	Acceleration = 0;
}

void ScreenLoading::Init()
{
	mLogo.SetImage(ImageLoader::LoadSkin("logo.png"));
	mLogo.Centered = true;
	mLogo.ColorInvert = true;
	mLogo.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	mLogo.SetSize(400);
	mLogo.SetCropToWholeImage();
	LoadThread = new boost::thread(LoadFunction, Next);
}

bool ScreenLoading::Run(double TimeDelta)
{
	if (!LoadThread)
		return RunNested(TimeDelta);

	Acceleration += 180 * TimeDelta;		 
	if (Acceleration > 720)
		Acceleration = 720;
	mLogo.AddRotation(TimeDelta * Acceleration);
	mLogo.Render();

	if (LoadThread->timed_join(boost::posix_time::seconds(0)))
	{
		delete LoadThread;
		LoadThread = NULL;
	}
	return true;
}

void ScreenLoading::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (!LoadThread)
		Next->HandleInput(key, code, isMouseInput);
}

void ScreenLoading::Cleanup()
{
}