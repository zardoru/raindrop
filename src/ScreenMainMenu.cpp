#include "Global.h"
#include "Screen.h"
#include "Song.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "FileManager.h"
#include "BitmapFont.h"
#include "ScreenMainMenu.h"
#include "ScreenSelectMusic.h"

SoundSample *MMSelectSnd = NULL;
BitmapFont* MainMenuFont = NULL;

ScreenMainMenu::ScreenMainMenu(IScreen *Parent) : IScreen(Parent)
{
}

void ScreenMainMenu::Init()
{
	ScreenTime = 0;
	Running = true;

	if (!MainMenuFont)
	{
		MainMenuFont = new BitmapFont();
		MainMenuFont->LoadSkinFontImage("font_screenevaluation.tga", glm::vec2(10, 20), glm::vec2(32, 32), glm::vec2(10,20), 32);
	}

	Background.SetImage(ImageLoader::LoadSkin("MenuBackground.png"));
	Logo.SetImage(ImageLoader::LoadSkin("logo.png"));
	PlayBtn.SetImage(ImageLoader::LoadSkin("playbtn.png"));
	ExitBtn.SetImage(ImageLoader::LoadSkin("exitbtn.png"));
	PlayBtn.AddPosition(ScreenWidth - PlayBtn.GetWidth(), 0);
	ExitBtn.AddPosition(ScreenWidth - ExitBtn.GetWidth(), PlayBtn.GetHeight());

	Logo.SetSize(480);
	Logo.Centered = true;
	Logo.SetPosition(Logo.GetWidth()/4, ScreenHeight - Logo.GetHeight()/4);

	if (!MMSelectSnd)
	{
		MMSelectSnd = new SoundSample((FileManager::GetSkinPrefix() + "select.ogg").c_str());
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
		MMSelectSnd->Reset();
		ScreenSelectMusic* NewScreen = new ScreenSelectMusic();
		NewScreen->Init();
		Next = NewScreen;
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

bool ScreenMainMenu::Run (double Delta)
{
	if (RunNested(Delta))
		return true;

	ScreenTime += Delta;
	Logo.SetScale( 1.0f + sin(ScreenTime)*sin(ScreenTime)*0.3 );

	Background.Render();
	Logo.Render();
	PlayBtn.Render();
	EditBtn.Render();
	OptionsBtn.Render();
	ExitBtn.Render();
	MainMenuFont->DisplayText("version: " DOTCUR_VERSIONTEXT "\nhttp://github.com/zardoru/dotcur", glm::vec2(0, 0));
	return Running;
}

void ScreenMainMenu::Cleanup()
{
}