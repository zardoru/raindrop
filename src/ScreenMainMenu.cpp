#include "GameGlobal.h"
#include "GameState.h"
#include "Configuration.h"
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
#include <glm/gtc/matrix_transform.inl>

SoundSample *MMSelectSnd = NULL;
BitmapFont* MainMenuFont = NULL;
LuaManager* MainMenuLua = NULL;
TruetypeFont* TTFO = NULL;

ScreenMainMenu::ScreenMainMenu(Screen *Parent) : Screen("ScreenMainMenu", Parent)
{
	TNext = nullptr;
}

void PlayBtnHover(Sprite *obj)
{
	MainMenuLua->CallFunction("PlayBtnHover");
	MainMenuLua->RunFunction();
}

void PlayBtnLeave(Sprite *obj)
{
	MainMenuLua->CallFunction("PlayBtnHoverLeave");
	MainMenuLua->RunFunction();
}

void ExitBtnHover(Sprite *obj)
{
	MainMenuLua->CallFunction("ExitBtnHover");
	MainMenuLua->RunFunction();
}

void ExitBtnLeave(Sprite *obj)
{
	MainMenuLua->CallFunction("ExitBtnHoverLeave");
	MainMenuLua->RunFunction();
}

void ScreenMainMenu::Init()
{
	Running = true;

	MainMenuLua = Animations->GetEnv();

	Animations->AddTarget(&Background);
	Animations->AddTarget(&PlayBtn);
	Animations->AddTarget(&ExitBtn);
	Animations->AddTarget(&OptionsBtn);
	Animations->AddTarget(&EditBtn);
	Animations->AddLuaTarget(&PlayBtn, "PlayButton");
	Animations->AddLuaTarget(&ExitBtn, "ExitButton");

	Animations->Initialize(GameState::GetInstance().GetSkinFile("mainmenu.lua"));
	Animations->InitializeUI();

	IntroDuration = Animations->GetIntroDuration();
	ExitDuration = Animations->GetIntroDuration();

	ChangeState(StateIntro);

	if (!TTFO)
		TTFO = new TruetypeFont(GameState::GetInstance().GetSkinFile("font.ttf"), 16);

	Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("MainMenuBackground")));
	Background.Centered = 1;
	Background.SetPosition( ScreenWidth / 2, ScreenHeight / 2 );
	
	PlayBtn.OnHover = PlayBtnHover;
	PlayBtn.OnLeave = PlayBtnLeave;
	ExitBtn.OnHover = ExitBtnHover;
	ExitBtn.OnLeave = ExitBtnLeave;

	Background.AffectedByLightning = true;

	if (!MMSelectSnd)
	{
		MMSelectSnd = new SoundSample();
		MMSelectSnd->Open((GameState::GetInstance().GetSkinFile("select.ogg")).c_str());
	}
}

bool ScreenMainMenu::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (Screen::HandleInput(key, code, isMouseInput))
		return true;

	if (PlayBtn.HandleInput(key, code, isMouseInput))
	{
		/* Use a screen loading to load selectmusic screen. */
		MMSelectSnd->Play();
		ChangeState(StateExit);
		return true;
	}
	else if(EditBtn.HandleInput(key, code, isMouseInput))
	{
		/* Create Select Music screen with Edit parameter = true */
	}
	else if(OptionsBtn.HandleInput(key, code, isMouseInput))
	{
		/* Create options screen. Run nested. */
	}
	else if (ExitBtn.HandleInput(key, code, isMouseInput))
	{
		Running = false;
	}

	return Animations->HandleInput(key, code, isMouseInput);
}

bool ScreenMainMenu::HandleScrollInput(double xOff, double yOff)
{
	return Screen::HandleScrollInput(xOff, yOff);
}

bool ScreenMainMenu::Run (double Delta)
{
	if (RunNested(Delta))
		return true;

	PlayBtn.Run(Delta);
	ExitBtn.Run(Delta);
	
	TTFO->Render (GString("version: " RAINDROP_VERSIONTEXT "\nhttp://github.com/zardoru/raindrop"), Vec2(0, 0), glm::translate(Mat4(), Vec3(0, 0, 16)));
	Animations->DrawTargets(Delta);

	return Running;
}

void ScreenMainMenu::OnExitEnd()
{
	Screen::OnExitEnd();

	Next = new ScreenSelectMusic();
	Next->Init();
	ChangeState(StateRunning);
	Animations->DoEvent("OnRestore");
}

void ScreenMainMenu::Cleanup()
{
}

