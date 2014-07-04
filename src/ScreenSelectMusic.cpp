#include <sstream>
#include <iomanip>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "GameGlobal.h"
#include "Screen.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "FileManager.h"
#include "Configuration.h"
#include "GraphObject2D.h"

#include "Song.h"
#include "ScreenSelectMusic.h"
#include "ScreenLoading.h"

#include "ScreenGameplay.h"
#include "ScreenGameplay7K.h"
#include "ScreenEdit.h"
#include "LuaManager.h"

#include "SongWheel.h"

#define SONGLIST_BASEY 120
#define SONGLIST_BASEX ScreenWidth*3/4

SoundSample *SelectSnd = NULL, *ClickSnd=NULL;
AudioStream	**Loops;
int LoopTotal;

ScreenSelectMusic::ScreenSelectMusic()
{
	Font = NULL;

	boost::function<float (float)> TransformFunc( boost::bind(&ScreenSelectMusic::GetListYTransformation, this, _1) );
	boost::function<void (Game::Song*, uint8)> SongNotifyFunc( boost::bind(&ScreenSelectMusic::OnSongChange, this, _1, _2) );
	boost::function<void (Game::Song*, uint8)> SongNotifySelectFunc( boost::bind(&ScreenSelectMusic::OnSongSelect, this, _1, _2) );
	Game::SongWheel::GetInstance().Initialize(0, 0, true, true, TransformFunc, SongNotifyFunc, SongNotifySelectFunc);

	if (!SelectSnd)
	{
		LoopTotal = 0;
		SelectSnd = new SoundSample();
		SelectSnd->Open((FileManager::GetSkinPrefix() + "select.ogg").c_str());
		MixerAddSample(SelectSnd);

		ClickSnd = new SoundSample();
		ClickSnd->Open((FileManager::GetSkinPrefix() + "click.ogg").c_str());
		MixerAddSample(ClickSnd);
		
		LoopTotal = Configuration::GetSkinConfigf("LoopTotal");

		Loops = new AudioStream*[LoopTotal];
		for (int i = 0; i < LoopTotal; i++)
		{
			std::stringstream str;
			str << FileManager::GetSkinPrefix() << "loop" << i+1 << ".ogg";
			Loops[i] = new AudioStream();
			Loops[i]->Open(str.str().c_str());
			Loops[i]->Stop();
			Loops[i]->SetLoop(true);
			MixerAddStream(Loops[i]);
		}
	}

	GameObject::GlobalInit();

	SelectedMode = MODE_7K;

	OptionUpscroll = false;
}

void ScreenSelectMusic::MainThreadInitialization()
{
	if (!Font)
	{
		Font = new BitmapFont();
		Font->LoadSkinFontImage("font_screenevaluation.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10,20), 32);
	}

	Objects->Initialize();

	Font->SetAffectedByLightning(true);

	Background.SetImage(ImageLoader::LoadSkin(Configuration::GetSkinConfigs("SelectMusicBackground")));

	Background.Centered = 1;
	Background.SetPosition( ScreenWidth / 2, ScreenHeight / 2 );

	WindowFrame.SetLightMultiplier(1);
	Background.AffectedByLightning = true;
}

void ScreenSelectMusic::LoadThreadInitialization()
{
	Running = true;

	SwitchBackGuiPending = true;
	char* Manifest[] =
	{
		(char*)Configuration::GetSkinConfigs("SelectMusicBackground").c_str(),
	};

	ImageLoader::LoadFromManifest(Manifest, 1, FileManager::GetSkinPrefix());

	Objects = new GraphObjectMan();
	Objects->Preload( FileManager::GetSkinPrefix() + "screenselectmusic.lua", "Preload" );

	Game::SongWheel::GetInstance().ReloadSongs();

	Time = 0;
}

void ScreenSelectMusic::Cleanup()
{
	delete Objects;
	StopLoops();
}

float ScreenSelectMusic::GetListYTransformation(const float Y)
{
	LuaManager *Lua = Objects->GetEnv();
	Lua->CallFunction("TransformList", 1, 1);
	Lua->PushArgument(Y);
	Lua->RunFunction();
	return Lua->GetFunctionResultF();
}

void ScreenSelectMusic::OnSongSelect(Game::Song* MySong, uint8 difindex)
{
	// Handle a recently selected song
	ScreenLoading *LoadNext = NULL;

	SelectSnd->Play();
	if (MySong->Mode == MODE_DOTCUR)
	{
		ScreenGameplay *DotcurGame = new ScreenGameplay(this);
		DotcurGame->Init(static_cast<dotcur::Song*>(MySong), difindex);

		LoadNext = new ScreenLoading(this, DotcurGame);
	}else
	{
		ScreenGameplay7K *VSRGGame = new ScreenGameplay7K();
		VSRGGame->Init(static_cast<VSRG::Song*>(MySong), difindex, OptionUpscroll);

		LoadNext = new ScreenLoading(this, VSRGGame);
	}

	LoadNext->Init();
	Next = LoadNext;
	StopLoops();
	SwitchBackGuiPending = true;
}

void ScreenSelectMusic::OnSongChange(Game::Song* MySong, uint8 difindex)
{
	ClickSnd->Play();
}

bool ScreenSelectMusic::Run(double Delta)
{
	if (RunNested(Delta))
		return true;
	else
	{
		if (SwitchBackGuiPending)
		{
			WindowFrame.isGuiInputEnabled = true;
			SwitchBackGuiPending = false;
			if (LoopTotal)
			{
				int rn = rand() % LoopTotal;
				Loops[rn]->SeekTime(0);
				Loops[rn]->Play();
			}
		}
	}

	Time += Delta;

	Game::SongWheel::GetInstance().Update(Delta);

	WindowFrame.SetLightMultiplier(sin(Time) * 0.2 + 1);

	Background.Render();
	Objects->DrawTargets(Delta);

	Game::SongWheel::GetInstance().Render();

	Font->DisplayText("song select", Vec2(ScreenWidth/2-55, 0));
	Font->DisplayText("press space to confirm", Vec2(ScreenWidth/2-110, 20));

	String modeString;

	if (OptionUpscroll)
		modeString = "upscroll";
	else
		modeString = "downscroll";


	Font->DisplayText(modeString.c_str(), Vec2(ScreenWidth/2-modeString.length() * 5, 40));

	return Running;
}	

void ScreenSelectMusic::StopLoops()
{
	for (int i = 0; i < LoopTotal; i++)
	{
		Loops[i]->SeekTime(0);
		Loops[i]->Stop();
	}
}

void ScreenSelectMusic::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (Next)
	{
		Next->HandleInput(key, code, isMouseInput);
		return;
	}

	Game::SongWheel::GetInstance().HandleInput(key, code, isMouseInput);

	if (code == KE_Press)
	{
		ScreenGameplay *_gNext = NULL;
		ScreenGameplay7K *_g7Next = NULL;

		ScreenEdit *_eNext = NULL;
		ScreenLoading *_LNext = NULL;

		switch (BindingsManager::TranslateKey(key))
		{
		case KT_Escape:
			Running = false;
			break;
		case KT_FractionDec:
			OptionUpscroll = !OptionUpscroll;
			break;
		}
	}
}

void ScreenSelectMusic::HandleScrollInput(double xOff, double yOff)
{
	if (Next)
		Next->HandleScrollInput(xOff, yOff);

	Game::SongWheel::GetInstance().HandleScrollInput(xOff, yOff);
}
