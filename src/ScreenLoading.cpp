#include "GameGlobal.h"
#include "GameState.h"
#include "ScreenLoading.h"
#include "LuaManager.h"
#include "GameWindow.h"
#include "BindingsManager.h"
#include "Logging.h"


void LoadFunction(void* pScreen)
{
	try {
		auto S = static_cast<Screen*>(pScreen);
		S->LoadThreadInitialization();
	} catch (std::exception &e)
	{
		Log::Printf("Exception occurred while loading: %s\n", e.what());
	}
}

ScreenLoading::ScreenLoading(Screen *Parent, Screen *_Next) : Screen("ScreenLoading", Parent)
{
	Next = _Next;
	LoadThread = nullptr;
	Running = true;
	ThreadInterrupted = false;

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

	// Close the screen we're loading if we asked to interrupt its loading.
	if (ThreadInterrupted)
		Next->Close();

	ChangeState(StateRunning);
}

bool ScreenLoading::Run(double TimeDelta)
{
	if (!LoadThread)
		return (Running = RunNested(TimeDelta));

	if (!Animations) return false;

	Animations->DrawTargets(TimeDelta);

	if (LoadThread->timed_join(boost::posix_time::millisec(16)))
	{
		delete LoadThread;
		LoadThread = nullptr;
		Next->MainThreadInitialization();
		ChangeState(StateExit);
	}

	return Running;
}

bool ScreenLoading::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (!LoadThread)
	{
		if (Next)
			return Next->HandleInput(key, code, isMouseInput);
		return true;
	}

	if (code == KE_Release)
	{
		if (BindingsManager::TranslateKey(key) == KT_Escape)
		{
			LoadThread->interrupt();
			ThreadInterrupted = true;
		}
	}

	return true;
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
