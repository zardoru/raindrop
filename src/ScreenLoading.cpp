#include "Global.h"
#include "GameState.h"
#include "Sprite.h"
#include "ScreenLoading.h"
#include "ImageLoader.h"
#include "LuaManager.h"
#include "GameWindow.h"


void LoadFunction(void* pScreen)
{
	Screen *S = (Screen*)pScreen;
	S->LoadThreadInitialization();
}

ScreenLoading::ScreenLoading(Screen *Parent, Screen *_Next) : Screen("ScreenLoading", Parent)
{
	Next = _Next;
	LoadThread = NULL;
	Running = true;

	GameState::GetInstance().InitializeLua(Animations->GetEnv()->GetState());

	Animations->Preload(GameState::GetInstance().GetSkinFile("screenloading.lua"), "Preload");
	Animations->Initialize("", false);

	IntroDuration = max(Animations->GetEnv()->GetGlobalD("IntroDuration"), 0.0);
	ExitDuration = max(Animations->GetEnv()->GetGlobalD("ExitDuration"), 0.0);
	
	ChangeState(StateIntro);
}

void ScreenLoading::OnIntroBegin()
{
	WindowFrame.SetLightMultiplier(0.8f);
	WindowFrame.SetLightPosition(glm::vec3(0, -0.5, 1));
}

void ScreenLoading::Init()
{
	LoadThread = new boost::thread(LoadFunction, Next);
}

void ScreenLoading::OnExitEnd()
{
	Screen::OnExitEnd();

	WindowFrame.SetLightMultiplier(1);
	WindowFrame.SetLightPosition(glm::vec3(0, 0, 1));

	Animations.reset();
	ChangeState(StateRunning);
}

bool ScreenLoading::Run(double TimeDelta)
{
	if (!LoadThread)
		return (Running = RunNested(TimeDelta));

	if (!Animations) return Running;

	Animations->DrawTargets(TimeDelta);

	if (LoadThread->timed_join(boost::posix_time::seconds(0)))
	{
		delete LoadThread;
		LoadThread = NULL;
		Next->MainThreadInitialization();
		ChangeState(StateExit);
	}
	boost::this_thread::sleep(boost::posix_time::millisec(16));
	return Running;
}

bool ScreenLoading::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (!LoadThread)
	{
		if (Next)
			return Next->HandleInput(key, code, isMouseInput);
	}

	return false;
}

bool ScreenLoading::HandleScrollInput(double xOff, double yOff)
{
	if (!LoadThread)
	{
		return Next->HandleScrollInput(xOff, yOff);
	}
	
	return Screen::HandleScrollInput(xOff, yOff);
}

void ScreenLoading::Cleanup()
{
}
