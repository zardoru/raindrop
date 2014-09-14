#include <sstream>
#include <iomanip>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "GameGlobal.h"
#include "GameState.h"
#include "Screen.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "Configuration.h"
#include "GraphObject2D.h"
#include "GuiButton.h"

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

static LuaManager* LuaMan;
int LoopTotal;

void OnUpHover(GraphObject2D* Obj)
{
	LuaMan->CallFunction("DirUpBtnHover");
	LuaMan->RunFunction();
}

void OnUpClick(GraphObject2D* Obj)
{
	LuaMan->CallFunction("DirUpBtnClick");
	LuaMan->RunFunction();
}

void OnUpHoverLeave(GraphObject2D* Obj)
{
	LuaMan->CallFunction("DirUpBtnHoverLeave");
	LuaMan->RunFunction();
}

void OnBackHover(GraphObject2D* Obj)
{
	LuaMan->CallFunction("BackBtnHover");
	LuaMan->RunFunction();
}

void OnBackClick(GraphObject2D* Obj)
{
	LuaMan->CallFunction("BackBtnClick");
	LuaMan->RunFunction();
}

void OnBackHoverLeave(GraphObject2D* Obj)
{
	LuaMan->CallFunction("BackBtnHoverLeave");
	LuaMan->RunFunction();
}

void OnAutoClick(GraphObject2D* Obj)
{
	LuaMan->CallFunction("AutoBtnClick");
	LuaMan->RunFunction();
}

void OnAutoHover(GraphObject2D* Obj)
{
	LuaMan->CallFunction("AutoBtnHover");
	LuaMan->RunFunction();
}

void OnAutoHoverLeave(GraphObject2D* Obj)
{
	LuaMan->CallFunction("AutoBtnHoverLeave");
	LuaMan->RunFunction();
}

ScreenSelectMusic::ScreenSelectMusic()
{
	Font = NULL;

	boost::function<float (float)> TransformFunc( boost::bind(&ScreenSelectMusic::GetListYTransformation, this, _1) );
	boost::function<void (Game::Song*, uint8)> SongNotifyFunc( boost::bind(&ScreenSelectMusic::OnSongChange, this, _1, _2) );
	boost::function<void (Game::Song*, uint8)> SongNotifySelectFunc( boost::bind(&ScreenSelectMusic::OnSongSelect, this, _1, _2) );
	Game::SongWheel::GetInstance().Initialize(0, 0, true, true, 
		TransformFunc, SongNotifyFunc, SongNotifySelectFunc, 
		GameState::GetInstance().GetSongDatabase());

	if (!SelectSnd)
	{
		LoopTotal = 0;
		SelectSnd = new SoundSample();
		SelectSnd->Open((GameState::GetInstance().GetSkinPrefix() + "select.ogg").c_str());
		MixerAddSample(SelectSnd);

		ClickSnd = new SoundSample();
		ClickSnd->Open((GameState::GetInstance().GetSkinPrefix() + "click.ogg").c_str());
		MixerAddSample(ClickSnd);
		
		LoopTotal = Configuration::GetSkinConfigf("LoopTotal");

		Loops = new AudioStream*[LoopTotal];
		for (int i = 0; i < LoopTotal; i++)
		{
			std::stringstream str;
			str << GameState::GetInstance().GetSkinPrefix() << "loop" << i+1 << ".ogg";
			Loops[i] = new AudioStream();
			Loops[i]->Open(str.str().c_str());
			Loops[i]->Stop();
			Loops[i]->SetLoop(true);
			MixerAddStream(Loops[i]);
		}
	}

	GameObject::GlobalInit();

	SelectedMode = MODE_7K;
}

void ScreenSelectMusic::MainThreadInitialization()
{
	if (!Font)
	{
		Font = new BitmapFont();
		Font->LoadSkinFontImage("font_screenevaluation.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10,20), 32);
	}
	
	UpBtn = new GUI::Button;
	UpBtn->OnClick = OnUpClick;
	UpBtn->OnHover = OnUpHover;
	UpBtn->OnLeave = OnUpHoverLeave;

	BackBtn = new GUI::Button;
	BackBtn->OnClick = OnBackClick;
	BackBtn->OnHover = OnBackHover;
	BackBtn->OnLeave = OnBackHoverLeave;

	AutoBtn = NULL;

	Objects->AddLuaTarget(BackBtn, "BackButton");
	Objects->AddLuaTarget(UpBtn, "DirUpButton");
	// Objects->AddLuaTarget(UpBtn, "AutoButton");
	Objects->AddTarget(BackBtn);
	Objects->AddTarget(UpBtn);
	// Objects->AddTarget(AutoBtn);

	Objects->Initialize();

	SwitchUpscroll(false);
	Font->SetAffectedByLightning(true);

	Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("SelectMusicBackground")));

	Background.Centered = 1;
	Background.SetPosition( ScreenWidth / 2, ScreenHeight / 2 );

	WindowFrame.SetLightMultiplier(1);
	Background.AffectedByLightning = true;
	Objects->AddLuaTarget(&Background, "ScreenBackground");
}

void ScreenSelectMusic::LoadThreadInitialization()
{
	Running = true;

	SwitchBackGuiPending = true;
	char* Manifest[] =
	{
		(char*)Configuration::GetSkinConfigs("SelectMusicBackground").c_str(),
	};

	ImageLoader::LoadFromManifest(Manifest, 1, GameState::GetInstance().GetSkinPrefix());

	Objects = new GraphObjectMan;
	Objects->Preload( GameState::GetInstance().GetSkinPrefix() + "screenselectmusic.lua", "Preload" );

	LuaMan = Objects->GetEnv();

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
		ScreenGameplay7K::Parameters Param;

		Param.Upscroll = OptionUpscroll;
		VSRGGame->Init(static_cast<VSRG::Song*>(MySong), difindex, Param);

		LoadNext = new ScreenLoading(this, VSRGGame);
	}

	LoadNext->Init();
	Next = LoadNext;
	StopLoops();

	Objects->GetEnv()->CallFunction("OnSelect");
	Objects->GetEnv()->RunFunction();

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

			Objects->GetEnv()->CallFunction("OnRestore");
			Objects->GetEnv()->RunFunction();
		}
	}

	Time += Delta;

	Game::SongWheel::GetInstance().Update(Delta);

	UpBtn->Run(Delta);
	BackBtn->Run(Delta);

	WindowFrame.SetLightMultiplier(sin(Time) * 0.2 + 1);

	Background.Render();
	Objects->UpdateTargets(Delta);

	Objects->DrawUntilLayer(16);

	Game::SongWheel::GetInstance().Render();

	Objects->DrawFromLayer(16);

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

void ScreenSelectMusic::SwitchUpscroll(bool NewUpscroll)
{
	OptionUpscroll = NewUpscroll;
	Objects->GetEnv()->SetGlobal("Upscroll", OptionUpscroll);
}

void ScreenSelectMusic::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (Next)
	{
		Next->HandleInput(key, code, isMouseInput);
		return;
	}

	if (Game::SongWheel::GetInstance().HandleInput(key, code, isMouseInput))
		return;

	if (UpBtn->HandleInput(key, code, isMouseInput))
		Game::SongWheel::GetInstance().GoUp();

	Objects->HandleInput(key, code, isMouseInput);

	if (code == KE_Press)
	{
		switch (BindingsManager::TranslateKey(key))
		{
		case KT_Escape:
			Running = false;
			break;
		case KT_FractionDec:
			SwitchUpscroll(!OptionUpscroll);
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
