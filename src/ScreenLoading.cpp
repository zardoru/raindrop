#include "Global.h"
#include "GraphObject2D.h"
#include "ScreenLoading.h"
#include "ImageLoader.h"
#include "GameWindow.h"
#include "FileManager.h"

void LoadFunction(void* pScreen)
{
	Screen *S = (Screen*)pScreen;
	S->LoadThreadInitialization();
}

ScreenLoading::ScreenLoading(Screen *Parent, Screen *_Next)
{
	Next = _Next;
	LoadThread = NULL;
	Running = true;
	
	Animation.Initialize(FileManager::GetSkinPrefix() + "screenloading.lua");
}

void ScreenLoading::Init()
{
	LoadThread = new boost::thread(LoadFunction, Next);
	WindowFrame.SetLightMultiplier(0.8f);
	WindowFrame.SetLightPosition(glm::vec3(0,-0.5,1));
}

bool ScreenLoading::Run(double TimeDelta)
{
	if (!LoadThread)
		return (Running = RunNested(TimeDelta));

	Animation.DrawTargets(TimeDelta);

	if (LoadThread->timed_join(boost::posix_time::seconds(0)))
	{
		delete LoadThread;
		LoadThread = NULL;
		WindowFrame.SetLightMultiplier(1);
		WindowFrame.SetLightPosition(glm::vec3(0,0,1));
		Next->MainThreadInitialization();
	}
	boost::this_thread::sleep(boost::posix_time::millisec(16));
	return Running;
}

void ScreenLoading::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (!LoadThread)
		if (Next)
			Next->HandleInput(key, code, isMouseInput);
}

void ScreenLoading::HandleScrollInput(double xOff, double yOff)
{
	if (!LoadThread)
		Next->HandleScrollInput(xOff, yOff);
}

void ScreenLoading::Cleanup()
{
}
