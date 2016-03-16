#include "pch.h"

#include "GameGlobal.h"
#include "GameState.h"
#include "Logging.h"
#include "Screen.h"
#include "SceneEnvironment.h"
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
#include "SongDatabase.h"

#include "SongWheel.h"

void LuaEvt(LuaManager* LuaMan, std::string Func, Sprite* Obj)
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
        .addFunction("GetIndexAtPoint", &SongWheel::IndexAtPoint)
        .addFunction("NormalizedIndexAtPoint", &SongWheel::NormalizedIndexAtPoint)
        .addFunction("GetTransformedY", &SongWheel::GetTransformedY)
        .addFunction("GoUp", &SongWheel::GoUp)
        .addFunction("AddSprite", &SongWheel::AddSprite)
        .addFunction("AddString", &SongWheel::AddText)
        .addFunction("ConfirmSelection", &SongWheel::ConfirmSelection)
        .addFunction("IsItemDirectory", &SongWheel::IsItemDirectory)
        .addProperty("SelectedIndex", &SongWheel::GetSelectedItem, &SongWheel::SetSelectedItem)
        .addProperty("ListY", &SongWheel::GetListY, &SongWheel::SetListY)
        .addProperty("CursorIndex", &SongWheel::GetCursorIndex, &SongWheel::SetCursorIndex)
        .addProperty("PendingY", &SongWheel::GetDeltaY, &SongWheel::SetDeltaY)
        .addProperty("ItemHeight", &SongWheel::GetItemHeight, &SongWheel::SetItemHeight)
        .addProperty("ItemWidth", &SongWheel::GetItemWidth, &SongWheel::SetItemWidth)
        .addProperty("ListIndex", &SongWheel::GetListCursorIndex)
        .addProperty("ItemCount", &SongWheel::GetNumItems)
        .addProperty("ItemHeight", &SongWheel::GetItemHeight)
        .addData("ScrollSpeed", &SongWheel::ScrollSpeed)
        .endClass();

    luabridge::push(L, &SongWheel::GetInstance());
    lua_setglobal(L, "Wheel");
}

ScreenSelectMusic::ScreenSelectMusic() : Screen("ScreenSelectMusic")
{
    PreviewStream = nullptr;

    PreviousPreview = std::make_shared<Game::Song>();
    ToPreview = nullptr;

    using Game::SongWheel;

    SongWheel * Wheel = &Game::SongWheel::GetInstance();
    Wheel->Initialize(GameState::GetInstance().GetSongDatabase());

    Game::SongNotification SongNotifyFunc(std::bind(&ScreenSelectMusic::OnSongChange, this, std::placeholders::_1, std::placeholders::_2));
    Game::SongNotification SongNotifySelectFunc(std::bind(&ScreenSelectMusic::OnSongSelect, this, std::placeholders::_1, std::placeholders::_2));
    Wheel->OnSongTentativeSelect = SongNotifyFunc;
    Wheel->OnSongConfirm = SongNotifySelectFunc;

    Game::ListTransformFunction TransformHFunc(std::bind(&ScreenSelectMusic::GetListHorizontalTransformation, this, std::placeholders::_1));
    Game::ListTransformFunction TransformVFunc(std::bind(&ScreenSelectMusic::GetListVerticalTransformation, this, std::placeholders::_1));
    Game::ListTransformFunction TransformPVert(std::bind(&ScreenSelectMusic::GetListPendingVerticalTransformation, this, std::placeholders::_1));
    Wheel->TransformHorizontal = TransformHFunc;
    Wheel->TransformListY = TransformVFunc;
    Wheel->TransformPendingDisplacement = TransformPVert;

    Game::DirectoryChangeNotifyFunction DirChangeNotif(std::bind(&ScreenSelectMusic::OnDirectoryChange, this));
    Wheel->OnDirectoryChange = DirChangeNotif;

    Game::ItemNotification ItClickNotif(std::bind(&ScreenSelectMusic::OnItemClick, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    Game::ItemNotification ItHoverNotif(std::bind(&ScreenSelectMusic::OnItemHover, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    Game::ItemNotification ItHoverLeaveNotif(std::bind(&ScreenSelectMusic::OnItemHoverLeave, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    Wheel->OnItemClick = ItClickNotif;
    Wheel->OnItemHover = ItHoverNotif;
    Wheel->OnItemHoverLeave = ItHoverLeaveNotif;

    Wheel->TransformItem = std::bind(&ScreenSelectMusic::TransformItem, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    Wheel->TransformString = std::bind(&ScreenSelectMusic::TransformString, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
	    
    SelectSnd = std::make_unique<SoundSample>();
    SelectSnd->Open(Configuration::GetSkinSound("SongSelectDecision"));

    ClickSnd = std::make_unique<SoundSample>();
    ClickSnd->Open(Configuration::GetSkinSound("SongSelectHover"));

    GameObject::GlobalInit();

    IsTransitioning = false;
    TransitionTime = 0;
}

void ScreenSelectMusic::InitializeResources()
{
    LuaManager* LuaM = Animations->GetEnv();
    UpBtn = std::make_unique<GUI::Button>();

    Animations->InitializeUI();

    EventAnimationFunction OnUpClick(std::bind(LuaEvt, LuaM, "DirUpBtnClick", std::placeholders::_1));
    EventAnimationFunction OnUpHover(std::bind(LuaEvt, LuaM, "DirUpBtnHover", std::placeholders::_1));
    EventAnimationFunction OnUpHoverLeave(std::bind(LuaEvt, LuaM, "DirUpBtnHoverLeave", std::placeholders::_1));
    UpBtn->OnClick = OnUpClick;
    UpBtn->OnHover = OnUpHover;
    UpBtn->OnLeave = OnUpHoverLeave;

    BackBtn = std::make_unique<GUI::Button>();

    EventAnimationFunction OnBackClick(std::bind(LuaEvt, LuaM, "BackBtnClick", std::placeholders::_1));
    EventAnimationFunction OnBackHover(std::bind(LuaEvt, LuaM, "BackBtnHover", std::placeholders::_1));
    EventAnimationFunction OnBackHoverLeave(std::bind(LuaEvt, LuaM, "BackBtnHoverLeave", std::placeholders::_1));
    BackBtn->OnClick = OnBackClick;
    BackBtn->OnHover = OnBackHover;
    BackBtn->OnLeave = OnBackHoverLeave;

    Animations->AddLuaTarget(BackBtn.get(), "BackButton");
    Animations->AddLuaTarget(UpBtn.get(), "DirUpButton");
    Animations->AddTarget(BackBtn.get());
    Animations->AddTarget(UpBtn.get());

    Animations->Initialize();

    GameState::GetInstance().InitializeLua(Animations->GetEnv()->GetState());

    Background.SetImage(GameState::GetInstance().GetSkinImage(Configuration::GetSkinConfigs("SelectMusicBackground")));

    Background.Centered = 1;
    Background.SetPosition(ScreenWidth / 2, ScreenHeight / 2);

    WindowFrame.SetLightMultiplier(1);
    Background.AffectedByLightning = true;
    Animations->AddLuaTarget(&Background, "ScreenBackground");
}

void ScreenSelectMusic::LoadResources()
{
    Running = true;

    SwitchBackGuiPending = true;

    SetupWheelLua(Animations->GetEnv());
    Animations->Preload(GameState::GetInstance().GetSkinFile("screenselectmusic.lua"), "Preload");

    Time = 0;
}

void ScreenSelectMusic::Cleanup()
{
    if (PreviewStream)
        PreviewStream = nullptr;

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
    std::shared_ptr<ScreenLoading> LoadNext;
    std::shared_ptr<Game::Song> MySong = GameState::GetInstance().GetSelectedSongShared();
    uint8_t difindex = GameState::GetInstance().GetDifficultyIndex();

    if (MySong->Mode == MODE_DOTCUR)
    {
        auto DotcurGame = std::make_shared<ScreenGameplay>();
        DotcurGame->Init(static_cast<dotcur::Song*>(MySong.get()), difindex);

        LoadNext = std::make_shared<ScreenLoading>(DotcurGame);
    }
    else
    {
        auto VSRGGame = std::make_shared<ScreenGameplay7K>();

        VSRGGame->Init(std::dynamic_pointer_cast<VSRG::Song>(MySong),
            difindex, *GameState::GetInstance().GetParameters());

        LoadNext = std::make_shared<ScreenLoading>(VSRGGame);
    }

    LoadNext->Init();
    Next = LoadNext;
}

void ScreenSelectMusic::OnSongSelect(std::shared_ptr<Game::Song> MySong, uint8_t difindex)
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

void ScreenSelectMusic::OnSongChange(std::shared_ptr<Game::Song> MySong, uint8_t difindex)
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
    std::string PreviewFile;

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
            PreviewStream = nullptr;
        }

        if (!PreviewStream)
        {
            auto previewPath = ToPreview->SongDirectory / PreviewFile;

            // If missing, find alternate preview file
            if (!std::filesystem::exists(previewPath))
                for (auto i : std::filesystem::directory_iterator(ToPreview->SongDirectory))
                {
                    auto extension = i.path().extension();
                    if (extension == ".mp3" || extension == ".ogg")
                        previewPath = i.path();
                }

            // Load preview
            if (std::filesystem::exists(previewPath)) {
                PreviewStream = std::make_shared<AudioStream>();
                if (PreviewStream->Open(previewPath.u8string().c_str()))
                {
                    PreviewStream->Play();
                    PreviewStream->SeekTime(StartTime);
                    PreviewStream->SetLoop(true);
                }
            }
        }
    }
    else
    {
        if (PreviewStream)
        {
            PreviewStream->Stop();
            PreviewStream = nullptr;
        }
    }

    PreviousPreview = ToPreview;
}

void ScreenSelectMusic::PlayLoops()
{
	if (!BGM) {
		auto fn = Configuration::GetSkinSound("SongSelectBGM");
		BGM = std::make_unique<SoundStream>();
		if (std::filesystem::exists(fn) &&
			std::filesystem::is_regular_file(fn)) {
			auto s = fn.string();
			auto IsLoop = false;
			Utility::ToLower(s);

			if (s.find_first_of("loop") != std::string::npos)
				IsLoop = true;

			if (BGM->Open(fn)) {
				BGM->SetLoop(IsLoop);
				BGM->Play();
			}
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
	if (BGM) {
		BGM->Stop();
		BGM = nullptr;
	}
}

bool ScreenSelectMusic::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
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
		default:
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

void ScreenSelectMusic::TransformItem(int Item, std::shared_ptr<Game::Song> Song, bool IsSelected, int Index)
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

void ScreenSelectMusic::TransformString(int Item, std::shared_ptr<Game::Song> Song, bool IsSelected, int Index, std::string text)
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

void ScreenSelectMusic::OnItemClick(int32_t Index, uint32_t boundIndex, std::string Line, std::shared_ptr<Game::Song> Selected)
{
    if (Animations->GetEnv()->CallFunction("OnItemClick", 4))
    {
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), boundIndex);
        luabridge::push(Animations->GetEnv()->GetState(), Line);
        luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenSelectMusic::OnItemHover(int32_t Index, uint32_t boundIndex, std::string Line, std::shared_ptr<Game::Song> Selected)
{
    if (Animations->GetEnv()->CallFunction("OnItemHover", 4))
    {
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), boundIndex);
        luabridge::push(Animations->GetEnv()->GetState(), Line);
        luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenSelectMusic::OnItemHoverLeave(int32_t Index, uint32_t boundIndex, std::string Line, std::shared_ptr<Game::Song> Selected)
{
    if (Animations->GetEnv()->CallFunction("OnItemHoverLeave", 4))
    {
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), boundIndex);
        luabridge::push(Animations->GetEnv()->GetState(), Line);
        luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
        Animations->GetEnv()->RunFunction();
    }
}