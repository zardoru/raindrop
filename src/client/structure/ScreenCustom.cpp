#include <filesystem>
#include <functional>

#include "Screen.h"
#include "ScreenCustom.h"
#include "SceneEnvironment.h"

ScreenCustom::ScreenCustom(GameWindow& window, const std::filesystem::path& ScriptName)
    : Screen(window, "ScreenCustom", false)
{
    scene_->initialize(ScriptName);
    intro_duration_ = scene_->get_intro_duration();
    exit_duration_ = scene_->get_exit_duration();
    change_state(StateIntro);
    is_active_ = true;

	scene_->set_screen_name(ScriptName.filename().replace_extension().string());
}

bool ScreenCustom::run(double Delta)
{
    // Update and draw targets, and carry on.
    scene_->draw_targets(Delta);
    return true;
}

bool ScreenCustom::on_input(int32_t key, bool isPressed, bool isMouseInput)
{
    return scene_->on_input(key, isPressed, isMouseInput);
}
