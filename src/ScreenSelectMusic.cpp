#include <sstream>

#include "GameGlobal.h"
#include "GameState.h"
#include "Logging.h"
#include "Screen.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "Configuration.h"
#include "Sprite.h"
#include "GuiButton.h"

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

void LuaEvt(LuaManager* LuaMan, GString Func, Sprite* Obj)
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
		.addFunction("IsLoading", &SongWheel::IsLoading)
		.addFunction("SetSelectedItem", &SongWheel::SetSelectedItem)
		.addFunction("GetSelectedItem", &SongWheel::GetSelectedItem)
		.addFunction("GetNumItems", &SongWheel::GetNumItems)
		.addFunction("GetItemHeight", &SongWheel::GetItemHeight)
		.addFunction("IndexAtPoint", &SongWheel::IndexAtPoint)
		.addFunction("NormalizedIndexAtPoint", &SongWheel::NormalizedIndexAtPoint)
		.addFunction("GetTransformedY", &SongWheel::GetTransformedY)
		.addFunction("GoUp", &SongWheel::GoUp)
		.addFunction("AddSprite", &SongWheel::AddSprite)
		.addFunction("AddString", &SongWheel::AddText)
		.addProperty("ListY", &SongWheel::GetListY, &SongWheel::SetListY)
		.addProperty("CursorIndex", &SongWheel::GetCursorIndex, &SongWheel::SetCursorIndex)
		.addProperty("PendingY", &SongWheel::GetDeltaY, &SongWheel::SetDeltaY)
		.addProperty("ItemHeight", &SongWheel::GetItemHeight, &SongWheel::SetItemHeight)
		.addProperty("ItemWidth", &SongWheel::GetItemWidth, &SongWheel::SetItemWidth)
		.addData("ScrollSpeed", &SongWheel::ScrollSpeed)
		.endClass();

	luabridge::push(L, &SongWheel::GetInstance());
	lua_setglobal(L, "Wheel");
}	

ScreenSelectMusic::ScreenSelectMusic() : Screen("ScreenSelectMusic")
{
	Font = nullptr;
	PreviewStream = nullptr;

	PreviousPreview = make_shared<Game::Song> ();
	ToPreview = nullptr;

	using Game::SongWheel;

	SongWheel * Wheel = &Game::SongWheel::GetInstance();
	Wheel->Initialize(GameState::GetInstance().GetSongDatabase());

	Game::SongNotification SongNotifyFunc(bind(&ScreenSelectMusic::OnSongChange, this, std::placeholders::_1, std::placeholders::_2));
	Game::SongNotification SongNotifySelectFunc(bind(&ScreenSelectMusic::OnSongSelect, this, std::placeholders::_1, std::placeholders::_2));
	Wheel->OnSongTentativeSelect = SongNotifyFunc;
	Wheel->OnSongConfirm = SongNotifySelectFunc;

	Game::ListTransformFunction TransformHFunc(bind(&ScreenSelectMusic::GetListHorizontalTransformation, this, std::placeholders::_1));
	Game::ListTransformFunction TransformVFunc(bind(&ScreenSelectMusic::GetListVerticalTransformation, this, std::placeholders::_1));
	Game::ListTransformFunction TransformPVert(bind(&ScreenSelectMusic::GetListPendingVerticalTransformation, this, std::placeholders::_1));
	Wheel->TransformHorizontal = TransformHFunc;
	Wheel->TransformListY = TransformVFunc;
	Wheel->TransformPendingDisplacement = TransformPVert;

	Game::DirectoryChangeNotifyFunction DirChangeNotif(bind(&ScreenSelectMusic::OnDirectoryChange, this));
	Wheel->OnDirectoryChange = DirChangeNotif;

	Game::ItemNotification ItClickNotif(bind(&ScreenSelectMusic::OnItemClick, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	Game::ItemNotification ItHoverNotif(bind(&ScreenSelectMusic::OnItemHover, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	Game::ItemNotification ItHoverLeaveNotif(bind(&ScreenSelectMusic::OnItemHoverLeave, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	Wheel->OnItemClick = ItClickNotif;
	Wheel->OnItemHover = ItHoverNotif;
	Wheel->OnItemHoverLeave = ItHoverLeaveNotif;

	Wheel->TransformItem = bind(&ScreenSelectMusic::TransformItem, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	Wheel->TransformString = bind(&ScreenSelectMusic::TransformString, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);

	if (!SelectSnd)
	{
		LoopTotal = 0;
		SelectSnd = new SoundSample();
		SelectSnd->Open((GameState::GetInstance().GetSkinFile("select.ogg")).c_str());

		ClickSnd = new SoundSample();
		ClickSnd->Open((GameState::GetInstance().GetSkinFile("click.ogg")).c_str());
		
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
		}
	}

	GameObject::GlobalInit();

	IsTransitioning = false;
	TransitionTime = 0;
}

void ScreenSelectMusic::MainThreadInitialization()
{	
	LuaManager* LuaM = Animations->GetEnv();
	UpBtn = new GUI::Button;

	Animations->InitializeUI();

	EventAnimationFunction OnUpClick (bind(LuaEvt, LuaM, "DirUpBtnClick",std::placeholders:: _1));
	EventAnimationFunction OnUpHover(bind(LuaEvt, LuaM, "DirUpBtnHover", std::placeholders::_1));
	EventAnimationFunction OnUpHoverLeave(bind(LuaEvt, LuaM, "DirUpBtnHoverLeave", std::placeholders::_1));
	UpBtn->OnClick = OnUpClick;
	UpBtn->OnHover = OnUpHover;
	UpBtn->OnLeave = OnUpHoverLeave;

	BackBtn = new GUI::Button;

	EventAnimationFunction OnBackClick(bind(LuaEvt, LuaM, "BackBtnClick", std::placeholders::_1));
	EventAnimationFunction OnBackHover(bind(LuaEvt, LuaM, "BackBtnHover", std::placeholders::_1));
	EventAnimationFunction OnBackHoverLeave(bind(LuaEvt, LuaM, "BackBtnHoverLeave", std::placeholders::_1));
	BackBtn->OnClick = OnBackClick;
	BackBtn->OnHover = OnBackHover;
	BackBtn->OnLeave = OnBackHoverLeave;

	Animations->AddLuaTarget(BackBtn, "BackButton");
	Animations->AddLuaTarget(UpBtn, "DirUpButton");
	Animations->AddTarget(BackBtn);
	Animations->AddTarget(UpBtn);

	Animations->Initialize();

	GameState::GetInstance().InitializeLua(Animations->GetEnv()->GetState());

	SwitchUpscroll(false);

	Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("SelectMusicBackground")));

	Background.Centered = 1;
	Background.SetPosition( ScreenWidth / 2, ScreenHeight / 2 );

	WindowFrame.SetLightMultiplier(1);
	Background.AffectedByLightning = true;
	Animations->AddLuaTarget(&Background, "ScreenBackground");
}

void ScreenSelectMusic::LoadThreadInitialization()
{
	Running = true;

	SwitchBackGuiPending = true;

	SetupWheelLua(Animations->GetEnv());
	Animations->Preload( GameState::GetInstance().GetSkinFile("screenselectmusic.lua"), "Preload" );

	Time = 0;
}

void ScreenSelectMusic::Cleanup()
{	
	if (PreviewStream)
		delete PreviewStream;

	StopLoops();

	GameState::GetInstance().SetSelectedSong(nullptr);
	Game::SongWheel::GetInstance().CleanItems();
}

float ScreenSelectMusic::GetListPendingVerticalTransformation(const float Y)
{
	LuaManager *Lua = Animations->GetEnv();
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
	LuaManager *Lua = Animations->GetEnv();
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
	LuaManager *Lua = Animations->GetEnv();
	if (Lua->CallFunction("TransformListHorizontal", 1, 1))
	{
		Lua->PushArgument(Y);
		Lua->RunFunction();
		return Lua->GetFunctionResultF();
	}
	else
		return 0;
}

void ScreenSelectMusic::StartGameplayScreen()
{
	ScreenLoading *LoadNext = nullptr;
	shared_ptr<Game::Song> MySong = GameState::GetInstance().GetSelectedSongShared();
	uint8 difindex = GameState::GetInstance().GetDifficultyIndex();

	if (MySong->Mode == MODE_DOTCUR)
	{
		ScreenGameplay *DotcurGame = new ScreenGameplay(this);
		DotcurGame->Init(static_cast<dotcur::Song*>(MySong.get()), difindex);

		LoadNext = new ScreenLoading(this, DotcurGame);
	}else
	{
		ScreenGameplay7K *VSRGGame = new ScreenGameplay7K();
		
		VSRGGame->Init(dynamic_pointer_cast<VSRG::Song>(MySong), 
		               difindex, *GameState::GetInstance().GetParameters());

		LoadNext = new ScreenLoading(this, VSRGGame);
	}

	LoadNext->Init();
	Next = LoadNext;
}

void ScreenSelectMusic::OnSongSelect(shared_ptr<Game::Song> MySong, uint8 difindex)
{
	// Handle a recently selected song

	if (IsTransitioning)
		return;

	if (PreviewStream) PreviewStream->Stop();

	IsTransitioning = true;

	SelectSnd->Play();

	StopLoops();

	GameState::GetInstance().SetSelectedSong(MySong);
	GameState::GetInstance().SetDifficultyIndex(difindex);

	Animations->DoEvent("OnSelect", 1);
	TransitionTime = Animations->GetEnv()->GetFunctionResultF();

	SwitchBackGuiPending = true;
}

void ScreenSelectMusic::OnSongChange(shared_ptr<Game::Song> MySong, uint8 difindex)
{
	ClickSnd->Play();

	if (MySong)
	{
		GameState::GetInstance().SetSelectedSong(MySong);
		Animations->DoEvent("OnSongChange");
		
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

	if (ToPreview == nullptr)
	{
		if (PreviewStream != nullptr)
			PreviewStream->Stop();
		return;
	}

	DB->GetPreviewInfo(ToPreview->ID, PreviewFile, StartTime);

	if (PreviewFile.length() > 0)
	{
		if (PreviewStream)
		{
			PreviewStream->Stop();
			delete PreviewStream;
			PreviewStream = nullptr;
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
		if (PreviewStream && PreviewStream->IsPlaying())
			PreviewStream->Stop();

		if (TransitionTime < 0)
		{
			if (RunNested(Delta))
				return true;
			else
			{
				IsTransitioning = false;
			}
		}
		else
		{
			// We're going to cross the threshold. Fire up the next screen.
			if (TransitionTime - Delta <= 0)
				StartGameplayScreen();

			TransitionTime -= Delta;
		}
	}
	else
	{
		if (SwitchBackGuiPending)
		{
			SwitchBackGuiPending = false;
			PlayLoops();
			Animations->DoEvent("OnRestore");
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
	}

	Time += Delta;

	Game::SongWheel::GetInstance().Update(Delta);

	UpBtn->Run(Delta);
	BackBtn->Run(Delta);

	WindowFrame.SetLightMultiplier(sin(Time) * 0.2 + 1);

	Background.Render();
	Animations->UpdateTargets(Delta);

	Animations->DrawUntilLayer(16);

	Game::SongWheel::GetInstance().Render();

	Animations->DrawFromLayer(16);

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
	Animations->GetEnv()->SetGlobal("Upscroll", OptionUpscroll);
}

bool ScreenSelectMusic::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (TransitionTime > 0 && IsTransitioning)
		return true;

	if (Next)
		return Next->HandleInput(key, code, isMouseInput);

	if (UpBtn->HandleInput(key, code, isMouseInput))
	{
		Game::SongWheel::GetInstance().GoUp();
		return true;
	}

	if (Game::SongWheel::GetInstance().HandleInput(key, code, isMouseInput))
		return true;

	Animations->HandleInput(key, code, isMouseInput);

	if (code == KE_PRESS)
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

	return true;
}

bool ScreenSelectMusic::HandleScrollInput(double xOff, double yOff)
{
	if (Next)
	{
		if (TransitionTime <= 0)
			return Next->HandleScrollInput(xOff, yOff);
		else
			return true;
	}

	Animations->HandleScrollInput(xOff, yOff);

	return Game::SongWheel::GetInstance().HandleScrollInput(xOff, yOff);
}


void ScreenSelectMusic::TransformItem(int Item, shared_ptr<Game::Song> Song, bool IsSelected, int Index)
{
	if (Animations->GetEnv()->CallFunction("TransformItem", 4))
	{
		luabridge::push(Animations->GetEnv()->GetState(), Item);
		luabridge::push(Animations->GetEnv()->GetState(), Song.get());
		luabridge::push(Animations->GetEnv()->GetState(), IsSelected);
		luabridge::push(Animations->GetEnv()->GetState(), Index);
		Animations->GetEnv()->RunFunction();
	}
}

void ScreenSelectMusic::TransformString(int Item, shared_ptr<Game::Song> Song, bool IsSelected, int Index, GString text)
{
	if (Animations->GetEnv()->CallFunction("TransformString", 5))
	{
		luabridge::push(Animations->GetEnv()->GetState(), Item);
		luabridge::push(Animations->GetEnv()->GetState(), Song.get());
		luabridge::push(Animations->GetEnv()->GetState(), IsSelected);
		luabridge::push(Animations->GetEnv()->GetState(), Index);
		luabridge::push(Animations->GetEnv()->GetState(), text.c_str());
		Animations->GetEnv()->RunFunction();
	}
}

void ScreenSelectMusic::OnDirectoryChange()
{
	Animations->DoEvent("OnDirectoryChange");
}

void ScreenSelectMusic::OnItemClick(uint32 Index, GString Line, shared_ptr<Game::Song> Selected)
{
	if (Animations->GetEnv()->CallFunction("OnItemClick", 3))
	{
		luabridge::push(Animations->GetEnv()->GetState(), Index);
		luabridge::push(Animations->GetEnv()->GetState(), Line);
		luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
		Animations->GetEnv()->RunFunction();
	}
}

void ScreenSelectMusic::OnItemHover(uint32 Index, GString Line, shared_ptr<Game::Song> Selected)
{
	if (Animations->GetEnv()->CallFunction("OnItemHover", 3))
	{
		luabridge::push(Animations->GetEnv()->GetState(), Index);
		luabridge::push(Animations->GetEnv()->GetState(), Line);
		luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
		Animations->GetEnv()->RunFunction();
	}
}

void ScreenSelectMusic::OnItemHoverLeave(uint32 Index, GString Line, shared_ptr<Game::Song> Selected)
{
	if (Animations->GetEnv()->CallFunction("OnItemHoverLeave", 3))
	{
		luabridge::push(Animations->GetEnv()->GetState(), Index);
		luabridge::push(Animations->GetEnv()->GetState(), Line);
		luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
		Animations->GetEnv()->RunFunction();
	}
}