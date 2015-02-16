#include "GameGlobal.h"
#include "GameState.h"
#include "Configuration.h"
#include "Screen.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "GameWindow.h"
#include "GraphObject2D.h"
#include "BitmapFont.h"
#include "TruetypeFont.h"

#include "Song.h"
#include "ScreenMainMenu.h"
#include "ScreenLoading.h"
#include "ScreenSelectMusic.h"
#include "LuaManager.h"

#include "RaindropRocketInterface.h"

SoundSample *MMSelectSnd = NULL;
BitmapFont* MainMenuFont = NULL;
LuaManager* MainMenuLua = NULL;
TruetypeFont* TTFO = NULL;

ScreenMainMenu::ScreenMainMenu(Screen *Parent) : Screen(Parent)
{
}

void PlayBtnHover(GraphObject2D *obj)
{
	MainMenuLua->CallFunction("PlayBtnHover");
	MainMenuLua->RunFunction();
}

void PlayBtnLeave(GraphObject2D *obj)
{
	MainMenuLua->CallFunction("PlayBtnHoverLeave");
	MainMenuLua->RunFunction();
}

void ExitBtnHover(GraphObject2D *obj)
{
	MainMenuLua->CallFunction("ExitBtnHover");
	MainMenuLua->RunFunction();
}

void ExitBtnLeave(GraphObject2D *obj)
{
	MainMenuLua->CallFunction("ExitBtnHoverLeave");
	MainMenuLua->RunFunction();
}

void ScreenMainMenu::Init()
{
	Running = true;

	Objects = new SceneManager("ScreenMainMenu");
	MainMenuLua = Objects->GetEnv();

	Objects->AddTarget(&Background);
	Objects->AddTarget(&PlayBtn);
	Objects->AddTarget(&ExitBtn);
	Objects->AddTarget(&OptionsBtn);
	Objects->AddTarget(&EditBtn);
	Objects->AddLuaTarget(&PlayBtn, "PlayButton");
	Objects->AddLuaTarget(&ExitBtn, "ExitButton");

	Objects->Initialize(GameState::GetInstance().GetSkinFile("mainmenu.lua"));

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
		MixerAddSample(MMSelectSnd);
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
		ScreenLoading *LoadScreen = new ScreenLoading(this, new ScreenSelectMusic());
		LoadScreen->Init();
		Next = LoadScreen;
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

	return Objects->HandleInput(key, code, isMouseInput);
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
	
	TTFO->Render (GString("version: " RAINDROP_VERSIONTEXT "\nhttp://github.com/zardoru/raindrop"), Vec2(0, 0));
	Objects->DrawTargets(Delta);

	return Running;
}

void ScreenMainMenu::Cleanup()
{
	delete Objects;
}