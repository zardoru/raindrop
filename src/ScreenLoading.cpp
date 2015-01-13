#include "Global.h"
#include "GameState.h"
#include "GraphObject2D.h"
#include "ScreenLoading.h"
#include "ImageLoader.h"
#include "LuaManager.h"
#include "GameWindow.h"


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
	
	Animation = new SceneManager("ScreenLoading");
	Animation->Preload(GameState::GetInstance().GetSkinFile("screenloading.lua"), "Preload");
	Animation->Initialize("", false);

	IntroDuration = max(Animation->GetEnv()->GetGlobalD("IntroDuration"), 0.0);
	ExitDuration = max(Animation->GetEnv()->GetGlobalD("ExitDuration"), 0.0);
	
	ChangeState(StateIntro);
}

void ScreenLoading::Init()
{
	LoadThread = new boost::thread(LoadFunction, Next);
	WindowFrame.SetLightMultiplier(0.8f);
	WindowFrame.SetLightPosition(glm::vec3(0,-0.5,1));
}

bool ScreenLoading::RunIntro(float Fraction)
{
	LuaManager *Lua = Animation->GetEnv();
	if (Lua->CallFunction("UpdateIntro", 1))
	{
		Lua->PushArgument(Fraction);
		Lua->RunFunction();
	}

	Animation->DrawFromLayer(0);
	return true;
}

bool ScreenLoading::RunExit(float Fraction)
{
	LuaManager *Lua = Animation->GetEnv();

	if (Fraction == 1)
	{
		delete Animation;
		Animation = NULL;
		ChangeState(StateRunning);
		return true;
	}

	if (Lua->CallFunction("UpdateExit", 1))
	{
		Lua->PushArgument(Fraction);
		Lua->RunFunction();
	}

	Animation->DrawFromLayer(0);
	return true;
}

bool ScreenLoading::Run(double TimeDelta)
{
	if (!LoadThread)
		return (Running = RunNested(TimeDelta));

	if (!Animation) return Running;

	Animation->DrawTargets(TimeDelta);

	if (LoadThread->timed_join(boost::posix_time::seconds(0)))
	{
		delete LoadThread;
		LoadThread = NULL;
		WindowFrame.SetLightMultiplier(1);
		WindowFrame.SetLightPosition(glm::vec3(0,0,1));
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
