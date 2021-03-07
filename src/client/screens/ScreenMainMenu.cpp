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
#include "ImageLoader.h"
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
ScreenMainMenu::ScreenMainMenu() : Screen("ScreenMainMenu", false)
{
    TNext = nullptr;
}

void ScreenMainMenu::Init()
{
    Running = true;

    MainMenuLua = Animations->GetEnv();
	/// Global gamestate instance.
	// @autoinstance Global
	GameState::GetInstance().InitializeLua(MainMenuLua->GetState());

    Animations->Initialize(GameState::GetInstance().GetSkinFile("mainmenu.lua"));
    Animations->InitializeUI();

    IntroDuration = Animations->GetIntroDuration();
    ExitDuration = Animations->GetIntroDuration();

    ChangeState(StateIntro);

    /*if (!TTFO)
        TTFO = new TruetypeFont(GameState::GetInstance().GetSkinFile("font.ttf"));
        */
}

bool ScreenMainMenu::HandleInput(int32_t key, bool isPressed, bool isMouseInput)
{
    if (Screen::HandleInput(key, isPressed, isMouseInput))
        return true;

    return Animations->HandleInput(key, isPressed, isMouseInput);
}

bool ScreenMainMenu::HandleScrollInput(double xOff, double yOff)
{
    return Screen::HandleScrollInput(xOff, yOff);
}

bool ScreenMainMenu::Run(double Delta)
{
    if (RunNested(Delta))
        return true;

    
    Animations->DrawTargets(Delta);

	/*
	float f = 24;
	auto m = glm::translate(0.f, 0.f, 30.f);
	TTFO->Render(std::string("version: " RAINDROP_VERSIONTEXT "\nhttp://github.com/zardoru/raindrop"), 
		Vec2(0, 0), m, Vec2(1, f));
	 */

    return Running;
}

void ScreenMainMenu::OnExitEnd()
{
    Screen::OnExitEnd();
    ChangeState(StateRunning);
    Animations->DoEvent("OnRestore");
}

void ScreenMainMenu::Cleanup()
{
}