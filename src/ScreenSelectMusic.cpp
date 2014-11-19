#include <sstream>
#include <iomanip>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "GameGlobal.h"
#include "GameState.h"
#include "Logging.h"
#include "Screen.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "Configuration.h"
#include "GraphObject2D.h"
#include "GuiButton.h"
#include "ImageList.h"

#include "Song.h"
#include "ScreenSelectMusic.h"
#include "ScreenLoading.h"

#include "ScreenGameplay.h"
#include "ScreenGameplay7K.h"
#include "ScreenEdit.h"
#include "LuaManager.h"
#include "LuaBridge.h"
#include "SongDatabase.h"

#include "SongWheel.h"

#define SONGLIST_BASEY 120
#define SONGLIST_BASEX ScreenWidth*3/4

SoundSample *SelectSnd = NULL, *ClickSnd=NULL;
AudioStream	**Loops;

int LoopTotal;

void LuaEvt(LuaManager* LuaMan, GString Func, GraphObject2D* Obj)
{
	LuaMan->CallFunction(Func.c_str());
	LuaMan->RunFunction();
}

void SetupWheelLua(LuaManager* Man)
{
	using namespace Game;
	lua_State* L = Man->GetState();
	luabridge::getGlobalNamespace(L)
		.beginClass<SongWheel>("SongWheel")
		.addFunction("NextDifficulty", &SongWheel::NextDifficulty)
		.addFunction("PrevDifficulty", &SongWheel::PrevDifficulty)
		.addFunction("SetDifficulty", &SongWheel::SetDifficulty)
		.addFunction("GetCursorIndex", &SongWheel::GetCursorIndex)
		.addFunction("SetSelectedItem", &SongWheel::SetSelectedItem)
		.addFunction("GetSelectedItem", &SongWheel::GetSelectedItem)
		.addFunction("GetNumItems", &SongWheel::GetNumItems)
		.addFunction("GetItemHeight", &SongWheel::GetItemHeight)
		.addFunction("IndexAtPoint", &SongWheel::IndexAtPoint)
		.addFunction("NormalizedIndexAtPoint", &SongWheel::NormalizedIndexAtPoint)
		.addFunction("GetTransformedY", &SongWheel::GetTransformedY)
		.addProperty("ListY", &SongWheel::GetListY, &SongWheel::SetListY)
		.addProperty("PendingY", &SongWheel::GetDeltaY, &SongWheel::SetDeltaY)
		.addData("ScrollSpeed", &SongWheel::ScrollSpeed)
		.endClass();

	luabridge::push(L, &Game::SongWheel::GetInstance());
	lua_setglobal(L, "Wheel");
}	

ScreenSelectMusic::ScreenSelectMusic()
{
	Font = NULL;
	PreviewStream = NULL;

	PreviousPreview = (Game::Song*)0xFFFFFFFF;
	ToPreview = NULL;

	Game::SongWheel * Wheel = &Game::SongWheel::GetInstance();
	Wheel->Initialize(0, 0, GameState::GetInstance().GetSongDatabase());

	boost::function<void(Game::Song*, uint8)> SongNotifyFunc(bind(&ScreenSelectMusic::OnSongChange, this, _1, _2));
	boost::function<void(Game::Song*, uint8)> SongNotifySelectFunc(bind(&ScreenSelectMusic::OnSongSelect, this, _1, _2));
	Wheel->OnSongTentativeSelect = SongNotifyFunc;
	Wheel->OnSongConfirm = SongNotifySelectFunc;

	Game::ListTransformFunction TransformHFunc(bind(&ScreenSelectMusic::GetListHorizontalTransformation, this, _1));
	Game::ListTransformFunction TransformVFunc(bind(&ScreenSelectMusic::GetListVerticalTransformation, this, _1));
	Game::ListTransformFunction TransformPVert(bind(&ScreenSelectMusic::GetListPendingVerticalTransformation, this, _1));
	Wheel->TransformHorizontal = TransformHFunc;
	Wheel->TransformListY = TransformVFunc;
	Wheel->TransformPendingDisplacement = TransformPVert;

	Game::DirectoryChangeNotifyFunction DirChangeNotif(bind(&ScreenSelectMusic::OnDirectoryChange, this));
	Wheel->OnDirectoryChange = DirChangeNotif;

	Game::ItemNotification ItClickNotif(bind(&ScreenSelectMusic::OnItemClick, this, _1, _2, _3));
	Game::ItemNotification ItHoverNotif(bind(&ScreenSelectMusic::OnItemHover, this, _1, _2, _3));
	Wheel->OnItemClick = ItClickNotif;
	Wheel->OnItemHover = ItHoverNotif;

	Wheel->TransformItem = bind(&ScreenSelectMusic::TransformItem, this, _1, _2, _3);

	if (!SelectSnd)
	{
		LoopTotal = 0;
		SelectSnd = new SoundSample();
		SelectSnd->Open((GameState::GetInstance().GetSkinFile("select.ogg")).c_str());
		MixerAddSample(SelectSnd);

		ClickSnd = new SoundSample();
		ClickSnd->Open((GameState::GetInstance().GetSkinFile("click.ogg")).c_str());
		MixerAddSample(ClickSnd);
		
		LoopTotal = Configuration::GetSkinConfigf("LoopTotal");

		Loops = new AudioStream*[LoopTotal];
		for (int i = 0; i < LoopTotal; i++)
		{
			std::stringstream str;
			str << "loop" << i+1 << ".ogg";
			Loops[i] = new AudioStream();
			Loops[i]->Open(GameState::GetInstance().GetSkinFile(str.str()).c_str());
			Loops[i]->Stop();
			Loops[i]->SetLoop(true);
			MixerAddStream(Loops[i]);
		}
	}

	GameObject::GlobalInit();

	IsTransitioning = false;
	TransitionTime = 0;
}

void ScreenSelectMusic::MainThreadInitialization()
{	
	LuaManager* LuaM = Objects->GetEnv();
	UpBtn = new GUI::Button;

	EventAnimationFunction OnUpClick (bind(LuaEvt, LuaM, "DirUpBtnClick", _1));
	EventAnimationFunction OnUpHover(bind(LuaEvt, LuaM, "DirUpBtnHover", _1));
	EventAnimationFunction OnUpHoverLeave(bind(LuaEvt, LuaM, "DirUpHoverLeave", _1));
	UpBtn->OnClick = OnUpClick;
	UpBtn->OnHover = OnUpHover;
	UpBtn->OnLeave = OnUpHoverLeave;

	BackBtn = new GUI::Button;

	EventAnimationFunction OnBackClick(bind(LuaEvt, LuaM, "BackBtnClick", _1));
	EventAnimationFunction OnBackHover(bind(LuaEvt, LuaM, "BackBtnHover", _1));
	EventAnimationFunction OnBackHoverLeave(bind(LuaEvt, LuaM, "BackBtnHoverLeave", _1));
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

	GameState::GetInstance().InitializeLua(Objects->GetEnv()->GetState());

	SwitchUpscroll(false);

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
	SetupWheelLua(Objects->GetEnv());
	Objects->Preload( GameState::GetInstance().GetSkinFile("screenselectmusic.lua"), "Preload" );

	Time = 0;
}

void ScreenSelectMusic::Cleanup()
{
	delete Objects;
	MixerRemoveStream(PreviewStream);
	StopLoops();
}

float ScreenSelectMusic::GetListPendingVerticalTransformation(const float Y)
{
	LuaManager *Lua = Objects->GetEnv();
	if (Lua->CallFunction("TransformPendingVertical", 1, 1))
	{
		Lua->PushArgument(Y);
		Lua->RunFunction();
		return Lua->GetFunctionResultF();
	}
	else return 0;
}

float ScreenSelectMusic::GetListVerticalTransformation(const float Y)
{
	LuaManager *Lua = Objects->GetEnv();
	if (Lua->CallFunction("TransformListVertical", 1, 1))
	{
		Lua->PushArgument(Y);
		Lua->RunFunction();
		return Lua->GetFunctionResultF();
	}
	else return 0;
}

float ScreenSelectMusic::GetListHorizontalTransformation(const float Y)
{
	LuaManager *Lua = Objects->GetEnv();
	if (Lua->CallFunction("TransformListHorizontal", 1, 1))
	{
		Lua->PushArgument(Y);
		Lua->RunFunction();
		return Lua->GetFunctionResultF();
	}
	else
		return 0;
}

void ScreenSelectMusic::OnSongSelect(Game::Song* MySong, uint8 difindex)
{
	// Handle a recently selected song
	ScreenLoading *LoadNext = NULL;

	if (IsTransitioning)
		return;

	if (PreviewStream) PreviewStream->Stop();

	IsTransitioning = true;

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

	GameState::GetInstance().SetSelectedSong(MySong);

	Objects->DoEvent("OnSelect", 1);
	TransitionTime = Objects->GetEnv()->GetFunctionResultF();

	SwitchBackGuiPending = true;
}

void ScreenSelectMusic::OnSongChange(Game::Song* MySong, uint8 difindex)
{
	ClickSnd->Play();

	if (MySong)
	{
		GameState::GetInstance().SetSelectedSong(MySong);
		GameState::GetInstance().SetDifficultyIndex(0);
		Objects->DoEvent("OnSongChange");
		
		PreviewWaitTime = 1;
	}

	ToPreview = MySong;
}

void ScreenSelectMusic::PlayPreview()
{
	// Do the song preview thing.
	SongDatabase* DB = GameState::GetInstance().GetSongDatabase();
	float StartTime;
	GString PreviewFile;

	if (ToPreview == NULL)
	{
		if (PreviewStream != NULL)
			PreviewStream->Stop();
		return;
	}

	DB->GetPreviewInfo(ToPreview->ID, PreviewFile, StartTime);

	if (PreviewFile.length() > 0)
	{
		if (PreviewStream)
		{
			PreviewStream->Stop();
			MixerRemoveStream(PreviewStream);
			delete PreviewStream;
			PreviewStream = NULL;
		}

		if (!PreviewStream)
		{
			Directory SDir = ToPreview->SongDirectory;
			PreviewStream = new AudioStream();

			if (PreviewStream->Open((SDir / PreviewFile).c_path()))
			{
				PreviewStream->Play();
				PreviewStream->SeekTime(StartTime);
				PreviewStream->SetLoop(true);
				MixerAddStream(PreviewStream);
			}
		}
	}

	PreviousPreview = ToPreview;
}

void ScreenSelectMusic::PlayLoops()
{
	if (LoopTotal)
	{
		bool PlayingAlready = false;
		for (int i = 0; i < LoopTotal; i++)
		{
			PlayingAlready = Loops[i]->IsPlaying();
			if (PlayingAlready) break;
		}

		if (!PlayingAlready)
		{
			int rn = rand() % LoopTotal;
			Loops[rn]->SeekTime(0);
			Loops[rn]->Play();
		}
	}
}

bool ScreenSelectMusic::Run(double Delta)
{
	if (IsTransitioning)
	{
		if (TransitionTime <= 0)
		{
			if (RunNested(Delta))
				return true;
			else
			{
				IsTransitioning = false;
			}
		}
		else
			TransitionTime -= Delta;
	}
	else
	{
		if (SwitchBackGuiPending)
		{
			SwitchBackGuiPending = false;
			PlayLoops();
			Objects->DoEvent("OnRestore");
		}
	}

	PreviewWaitTime -= Delta;
	if (PreviewWaitTime <= 0)
	{
		if (PreviousPreview != ToPreview)
			PlayPreview();

		if (PreviewStream && PreviewStream->IsPlaying())
			StopLoops();
		else
		{
			if (!SwitchBackGuiPending)
				PlayLoops();
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
		if (TransitionTime <= 0)
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
		case KT_Left:
			GameState::GetInstance().SetDifficultyIndex(Game::SongWheel::GetInstance().PrevDifficulty());
			break;
		case KT_Right:
			GameState::GetInstance().SetDifficultyIndex(Game::SongWheel::GetInstance().NextDifficulty());
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
	{
		if (TransitionTime <= 0)
			Next->HandleScrollInput(xOff, yOff);
		else
			return;
	}

	Game::SongWheel::GetInstance().HandleScrollInput(xOff, yOff);
}

void ScreenSelectMusic::TransformItem(GraphObject2D* Item, Game::Song* Song, bool IsSelected)
{
	if (Objects->GetEnv()->CallFunction("TransformItem", 3))
	{
		luabridge::push(Objects->GetEnv()->GetState(), Item);
		luabridge::push(Objects->GetEnv()->GetState(), Song);
		luabridge::push(Objects->GetEnv()->GetState(), IsSelected);
		Objects->GetEnv()->RunFunction();
	}
}

void ScreenSelectMusic::OnDirectoryChange()
{
	Objects->DoEvent("OnDirectoryChange");
}

void ScreenSelectMusic::OnItemClick(uint32 Index, GString Line, Game::Song* Selected)
{
	if (Objects->GetEnv()->CallFunction("OnItemClick", 3))
	{
		luabridge::push(Objects->GetEnv()->GetState(), Index);
		luabridge::push(Objects->GetEnv()->GetState(), Line);
		luabridge::push(Objects->GetEnv()->GetState(), Selected);
		Objects->GetEnv()->RunFunction();
	}
}

void ScreenSelectMusic::OnItemHover(uint32 Index, GString Line, Game::Song* Selected)
{
	if (Objects->GetEnv()->CallFunction("OnItemHover", 3))
	{
		luabridge::push(Objects->GetEnv()->GetState(), Index);
		luabridge::push(Objects->GetEnv()->GetState(), Line);
		luabridge::push(Objects->GetEnv()->GetState(), Selected);
		Objects->GetEnv()->RunFunction();
	}
}