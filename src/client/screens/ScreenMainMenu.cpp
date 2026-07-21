#include <filesystem>
#include <map>
#include <functional>

#include <glm.h>
#include <rmath.h>

#include <game/GameConstants.h>
#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"

#include "../structure/SceneEnvironment.h"
#include "../structure/Screen.h"
#include "TextureCollection.h"
#include "Audio.h"
#include "GameWindow.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "Font.h"
#include "BitmapFont.h"
#include "TruetypeFont.h"

#include "ScreenMainMenu.h"
// #include "ScreenLoading.h"
// #include "ScreenSelectMusic.h"
#include "LuaManager.h"

// fixme: rmlui
// #include "RaindropRocketInterface.h"
//#include <glm/gtc/matrix_transform.inl>

AudioSample *MMSelectSnd = NULL;
BitmapFont* MainMenuFont = NULL;
LuaManager* MainMenuLua = NULL;


/// @themescript mainmenu.lua
ScreenMainMenu::ScreenMainMenu(GameWindow& window) : Screen(window, "ScreenMainMenu", false)
{
    TNext = nullptr;
}

void ScreenMainMenu::init()
{
    is_active_ = true;

    MainMenuLua = scene_->get_script_manager();
	/// Global gamestate instance.
	// @autoinstance Global
	GameState::get_instance().initialize_lua(MainMenuLua->get_lua_state());

    scene_->initialize(GameState::get_instance().get_skin_file("mainmenu.lua"));

    intro_duration_ = scene_->get_intro_duration();
    exit_duration_ = scene_->get_intro_duration();

    change_state(StateIntro);

    /*if (!TTFO)
        TTFO = new TruetypeFont(GameState::GetInstance().GetSkinFile("font.ttf"));
        */
}

bool ScreenMainMenu::on_input(int32_t key, bool isPressed, bool isMouseInput)
{
    if (Screen::on_input(key, isPressed, isMouseInput))
        return true;

    return scene_->on_input(key, isPressed, isMouseInput);
}

bool ScreenMainMenu::on_scroll_input(double xOff, double yOff)
{
    return Screen::on_scroll_input(xOff, yOff);
}

bool ScreenMainMenu::run(double Delta)
{
    if (run_nested(Delta))
        return true;

    
    scene_->draw_targets(Delta);

	/*
	float f = 24;
	auto m = glm::translate(0.f, 0.f, 30.f);
	TTFO->Render(std::string("version: " RAINDROP_VERSIONTEXT "\nhttp://github.com/zardoru/raindrop"), 
		Vec2(0, 0), m, Vec2(1, f));
	 */

    return is_active_;
}

void ScreenMainMenu::on_exit_end()
{
    Screen::on_exit_end();
    change_state(StateRunning);
    scene_->trigger_event("OnRestore");
}

void ScreenMainMenu::cleanup()
{
}
