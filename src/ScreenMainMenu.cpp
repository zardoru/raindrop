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

	Objects = new GraphObjectMan();
	MainMenuLua = Objects->GetEnv();

	Objects->AddTarget(&Background);
	Objects->AddTarget(&PlayBtn);
	Objects->AddTarget(&ExitBtn);
	Objects->AddTarget(&OptionsBtn);
	Objects->AddTarget(&EditBtn);
	Objects->AddLuaTarget(&PlayBtn, "PlayButton");
	Objects->AddLuaTarget(&ExitBtn, "ExitButton");
	Objects->Initialize(GameState::GetInstance().GetSkinPrefix() + "mainmenu.lua");

	/*if (!MainMenuFont)
	{
		MainMenuFont = new BitmapFont();
		MainMenuFont->LoadSkinFontImage("font_screenevaluation.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10,20), 32);
		MainMenuFont->SetAffectedByLightning(true);	
	}*/

	if (!TTFO)
	{
		TTFO = new TruetypeFont("GameData/r.ttf");
	}

	Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("MainMenuBackground")));
	Background.Centered = 1;
	Background.SetPosition( ScreenWidth / 2, ScreenHeight / 2 );
	
	PlayBtn.SetImage(GameState::GetInstance().GetSkinImage("play.png"), false);
	ExitBtn.SetImage(GameState::GetInstance().GetSkinImage("quit.png"), false);
	PlayBtn.OnHover = PlayBtnHover;
	PlayBtn.OnLeave = PlayBtnLeave;
	ExitBtn.OnHover = ExitBtnHover;
	ExitBtn.OnLeave = ExitBtnLeave;

	Background.AffectedByLightning = true;

	if (!MMSelectSnd)
	{
		MMSelectSnd = new SoundSample();
		MMSelectSnd->Open((GameState::GetInstance().GetSkinPrefix() + "select.ogg").c_str());
		MixerAddSample(MMSelectSnd);
	}
}

void ScreenMainMenu::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (Next && Next->IsScreenRunning())
	{
		Next->HandleInput(key, code, isMouseInput);
		return;
	}

	if (PlayBtn.HandleInput(key, code, isMouseInput))
	{
		/* Use a screen loading to load selectmusic screen. */
		MMSelectSnd->Play();
		ScreenLoading *LoadScreen = new ScreenLoading(this, new ScreenSelectMusic());
		LoadScreen->Init();
		Next = LoadScreen;
		return;
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
}

void ScreenMainMenu::HandleScrollInput(double xOff, double yOff)
{
	if (Next)
		Next->HandleScrollInput(xOff, yOff);
}

bool ScreenMainMenu::Run (double Delta)
{
	if (RunNested(Delta))
		return true;

	PlayBtn.Run(Delta);
	ExitBtn.Run(Delta);
	
	Objects->DrawTargets(Delta);

	// MainMenuFont->DisplayText("version: " RAINDROP_VERSIONTEXT "\nhttp://github.com/zardoru/raindrop", Vec2(0, 0));
	TTFO->Render ("version: " RAINDROP_VERSIONTEXT "\nhttp://github.com/zardoru/raindrop", Vec2(0, 0), 18);
	return Running;
}

void ScreenMainMenu::Cleanup()
{
	delete Objects;
}