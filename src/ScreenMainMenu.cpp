#include "GameGlobal.h"
#include "Configuration.h"
#include "Screen.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "FileManager.h"
#include "GameWindow.h"
#include "GraphObject2D.h"
#include "BitmapFont.h"

#include "Song.h"
#include "ScreenMainMenu.h"
#include "ScreenLoading.h"
#include "ScreenSelectMusic.h"
#include "LuaManager.h"

SoundSample *MMSelectSnd = NULL;
BitmapFont* MainMenuFont = NULL;
LuaManager* MainMenuLua = NULL;

ScreenMainMenu::ScreenMainMenu(IScreen *Parent) : IScreen(Parent)
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
	ScreenTime = 0;
	Running = true;

	Objects = new GraphObjectMan();
	MainMenuLua = Objects->GetEnv();

	Objects->AddTarget(&Background);
	Objects->AddTarget(&PlayBtn);
	Objects->AddTarget(&ExitBtn);
	Objects->AddLuaTarget(&PlayBtn, "PlayButton");
	Objects->AddLuaTarget(&ExitBtn, "ExitButton");
	Objects->Initialize(FileManager::GetSkinPrefix() + "mainmenu.lua");

	if (!MainMenuFont)
	{
		MainMenuFont = new BitmapFont();
		MainMenuFont->LoadSkinFontImage("font_screenevaluation.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10,20), 32);
		MainMenuFont->SetAffectedByLightning(true);	
	}

	Background.SetImage(ImageLoader::LoadSkin(Configuration::GetSkinConfigs("MainMenuBackground")));
	Background.Centered = 1;
	Background.SetPosition( ScreenWidth / 2, ScreenHeight / 2 );
	
	PlayBtn.SetImage(ImageLoader::LoadSkin("play.png"), false);
	ExitBtn.SetImage(ImageLoader::LoadSkin("quit.png"), false);
	PlayBtn.OnHover = PlayBtnHover;
	PlayBtn.OnLeave = PlayBtnLeave;
	ExitBtn.OnHover = ExitBtnHover;
	ExitBtn.OnLeave = ExitBtnLeave;

	Background.AffectedByLightning = true;

	if (!MMSelectSnd)
	{
		MMSelectSnd = new SoundSample();
		MMSelectSnd->Open((FileManager::GetSkinPrefix() + "select.ogg").c_str());
		MixerAddSample(MMSelectSnd);
	}

	//WindowFrame.SetLightMultiplier(800);
	//WindowFrame.SetLightPosition(glm::vec3(ScreenWidth / 2, ScreenHeight / 2, 1));
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

	ScreenTime += Delta;

	PlayBtn.Run(Delta);
	ExitBtn.Run(Delta);

	Background.Render();
	
	Objects->DrawUntilLayer(16);

	EditBtn.Render();
	OptionsBtn.Render();

	Objects->DrawFromLayer(17);

	MainMenuFont->DisplayText("version: " RAINDROP_VERSIONTEXT "\nhttp://github.com/zardoru/raindrop", Vec2(0, 0));
	return Running;
}

void ScreenMainMenu::Cleanup()
{
	delete Objects;
}