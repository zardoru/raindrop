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
    next_screen_ = _Next;
    LoadThread = nullptr;
    is_active_ = true;
    ThreadInterrupted = false;
	/// Global gamestate.
	// @autoinstance Global
    GameState::get_instance().initialize_lua(scene_->get_script_manager()->get_lua_state());

    scene_->preload(GameState::get_instance().get_skin_file("screenloading.lua"), "Preload");
    scene_->initialize("", false);

    intro_duration_ = std::max(scene_->get_script_manager()->get_global_d("IntroDuration"), 0.0);
    exit_duration_ = std::max(scene_->get_script_manager()->get_global_d("ExitDuration"), 0.0);

    change_state(StateIntro);
}

void ScreenLoading::on_intro_begin()
{
    //WindowFrame.SetLightMultiplier(0.8f);
    //WindowFrame.SetLightPosition(glm::vec3(0, -0.5, 1));
}

void ScreenLoading::init()
{
    LoadThread = std::make_shared<std::thread>(&LoadScreenThread::DoLoad, LoadScreenThread(FinishedLoading, next_screen_.get()));
}

void ScreenLoading::on_exit_end()
{
    Screen::on_exit_end();

    //WindowFrame.SetLightMultiplier(1);
    //WindowFrame.SetLightPosition(glm::vec3(0, 0, 1));

    scene_.reset();

    // Close the screen we're loading if we asked to interrupt its loading.
    if (ThreadInterrupted)
        next_screen_->close();
	
    change_state(StateRunning);
}

bool ScreenLoading::run(double TimeDelta)
{
    if (!LoadThread && !ThreadInterrupted)
        return (is_active_ = run_nested(TimeDelta));

    if (!scene_) return false;

    scene_->draw_targets(TimeDelta);

    if (FinishedLoading)
    {
        LoadThread->join();
        LoadThread = nullptr;
        next_screen_->post_load_initialization();
        change_state(StateExit);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    return is_active_;
}

bool ScreenLoading::on_input(int32_t key, bool isPressed, bool isMouseInput)
{
    if (!LoadThread)
    {
        if (next_screen_)
            return next_screen_->on_input(key, isPressed, isMouseInput);
        return true;
    }

    if (!isPressed)
    {
        if (BindingsManager::translate_key(key) == KT_Escape)
        {
            next_screen_->RequestInterrupt();
            ThreadInterrupted = true;
        }
    }

    return true;
}

bool ScreenLoading::on_scroll_input(double xOff, double yOff)
{
    if (!LoadThread)
    {
        return next_screen_->on_scroll_input(xOff, yOff);
    }

    return Screen::on_scroll_input(xOff, yOff);
}

void ScreenLoading::cleanup()
{
}