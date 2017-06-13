#include "pch.h"


#include "GameState.h"

#include "SceneEnvironment.h"
#include "Screen.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "GameWindow.h"
#include "Sprite.h"
#include "BitmapFont.h"
#include "TruetypeFont.h"

#include "Song.h"
#include "ScreenMainMenu.h"
#include "ScreenLoading.h"
#include "ScreenSelectMusic.h"
#include "LuaManager.h"

#include "RaindropRocketInterface.h"
//#include <glm/gtc/matrix_transform.inl>

AudioSample *MMSelectSnd = NULL;
BitmapFont* MainMenuFont = NULL;
LuaManager* MainMenuLua = NULL;
TruetypeFont* TTFO = NULL;

ScreenMainMenu::ScreenMainMenu() : Screen("ScreenMainMenu", nullptr)
{
    TNext = nullptr;
}

void ScreenMainMenu::Init()
{
    Running = true;

    MainMenuLua = Animations->GetEnv();
	GameState::GetInstance().InitializeLua(MainMenuLua->GetState());

    Animations->Initialize(GameState::GetInstance().GetSkinFile("mainmenu.lua"));
    Animations->InitializeUI();

    IntroDuration = Animations->GetIntroDuration();
    ExitDuration = Animations->GetIntroDuration();

    ChangeState(StateIntro);

    if (!TTFO)
        TTFO = new TruetypeFont(GameState::GetInstance().GetSkinFile("font.ttf"));
}

bool ScreenMainMenu::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
{
    if (Screen::HandleInput(key, code, isMouseInput))
        return true;

    return Animations->HandleInput(key, code, isMouseInput);
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

	float f = 16 / SDF_SIZE;
	auto m = glm::translate(0.f, 0.f, 30.f);
	TTFO->Render(std::string("version: " RAINDROP_VERSIONTEXT "\nhttp://github.com/zardoru/raindrop"), 
		Vec2(0, 0), m, Vec2(f, f));
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