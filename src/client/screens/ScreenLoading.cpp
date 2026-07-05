#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include <filesystem>
#include <map>
#include <functional>

#include <game/GameConstants.h>
#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"

#include "../structure/Screen.h"
#include "../structure/SceneEnvironment.h"

#include "LuaManager.h"
#include "ScreenLoading.h"
#include "Logging.h"
#include "../game/Game.h"

/// @themescript screenloading.lua

class LoadScreenThread
{
    std::atomic<bool>& mFinished;
    Screen* mScreen;
public:
    LoadScreenThread(std::atomic<bool>& status, Screen* screen) : mFinished(status), mScreen(screen) {};
    void DoLoad() const
    {
        mFinished = false;
        try
        {
            mScreen->load_resources();
        }
        catch (InterruptedException &)
        {
            Log::Printf("Thread was interrupted.\n");
        }
        catch (std::exception &e)
        {
            Log::LogPrintf("Exception while loading: %s\n", e.what());
        }
        mFinished = true;
    }
};

ScreenLoading::ScreenLoading(std::shared_ptr<Screen> _Next) : Screen("ScreenLoading", false)
{
    Next = _Next;
    LoadThread = nullptr;
    is_active_ = true;
    ThreadInterrupted = false;
	/// Global gamestate.
	// @autoinstance Global
    GameState::get_instance().initialize_lua(scene_->get_script_manager()->get_lua_state());

    scene_->Preload(GameState::get_instance().get_skin_file("screenloading.lua"), "Preload");
    scene_->Initialize("", false);

    IntroDuration = std::max(scene_->get_script_manager()->GetGlobalD("IntroDuration"), 0.0);
    ExitDuration = std::max(scene_->get_script_manager()->GetGlobalD("ExitDuration"), 0.0);

    ChangeState(StateIntro);
}

void ScreenLoading::OnIntroBegin()
{
    //WindowFrame.SetLightMultiplier(0.8f);
    //WindowFrame.SetLightPosition(glm::vec3(0, -0.5, 1));
}

void ScreenLoading::Init()
{
    LoadThread = std::make_shared<std::thread>(&LoadScreenThread::DoLoad, LoadScreenThread(FinishedLoading, Next.get()));
}

void ScreenLoading::OnExitEnd()
{
    Screen::OnExitEnd();

    //WindowFrame.SetLightMultiplier(1);
    //WindowFrame.SetLightPosition(glm::vec3(0, 0, 1));

    scene_.reset();

    // Close the screen we're loading if we asked to interrupt its loading.
    if (ThreadInterrupted)
        Next->Close();
	
    ChangeState(StateRunning);
}

bool ScreenLoading::Run(double TimeDelta)
{
    if (!LoadThread && !ThreadInterrupted)
        return (is_active_ = RunNested(TimeDelta));

    if (!scene_) return false;

    scene_->DrawTargets(TimeDelta);

    if (FinishedLoading)
    {
        LoadThread->join();
        LoadThread = nullptr;
        Next->post_load_initialization();
        ChangeState(StateExit);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    return is_active_;
}

bool ScreenLoading::HandleInput(int32_t key, bool isPressed, bool isMouseInput)
{
    if (!LoadThread)
    {
        if (Next)
            return Next->HandleInput(key, isPressed, isMouseInput);
        return true;
    }

    if (!isPressed)
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