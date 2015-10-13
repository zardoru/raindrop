#include "GameGlobal.h"
#include "GameState.h"
#include "Screen.h"
#include "SceneEnvironment.h"
#include "LuaManager.h"
#include "ScreenLoading.h"
#include "GameWindow.h"
#include "BindingsManager.h"
#include "Logging.h"


class LoadScreenThread{
	atomic<bool>& mFinished;
	Screen* mScreen;
public:
	LoadScreenThread(atomic<bool>& status, Screen* screen) : mFinished(status), mScreen(screen) {};
	void DoLoad()
	{
		mFinished = false;
		try {
			mScreen->LoadResources();
		}
		catch (InterruptedException &)
		{
			Log::Printf("Thread was interrupted.\n");
		}catch(std::exception &e)
		{
			Log::LogPrintf("Exception while loading: %s\n", e.what());
		}
		mFinished = true;
	}
};

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
	LoadThread = new thread(&LoadScreenThread::DoLoad, LoadScreenThread(FinishedLoading, Next));
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

	if (FinishedLoading)
	{
		LoadThread->join();
		delete LoadThread;
		LoadThread = nullptr;
		Next->InitializeResources();
		ChangeState(StateExit);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(16));
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

	if (code == KE_RELEASE)
	{
		if (BindingsManager::TranslateKey(key) == KT_Escape)
		{
			Next->RequestInterrupt();
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
