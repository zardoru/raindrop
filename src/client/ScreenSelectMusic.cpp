#include <string>
#include <memory>
#include <filesystem>
#include <thread>
#include <queue>
#include <future>
#include <json.hpp>
#include <glm.h>
#include <rmath.h>

#include <game/Song.h>
#include "PlayscreenParameters.h"
#include "GameState.h"
#include "Logging.h"
#include "Screen.h"
#include "SceneEnvironment.h"
#include "GameWindow.h"
#include "ImageLoader.h"

#include <Audio.h>
#include <Audiofile.h>
#include <AudioSourceOJM.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "Line.h"

#include "ScreenSelectMusic.h"
#include "ScreenLoading.h"

#include "BackgroundAnimation.h"

#include "ScreenGameplay7K.h"

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>
#include <TextAndFileUtil.h>

#include "SongDatabase.h"
#include "SongList.h"
#include "SongWheel.h"

#include "Configuration.h"
#include "Game.h"

void LuaEvt(LuaManager *LuaMan, std::string Func, Sprite *Obj) {
    LuaMan->CallFunction(Func.c_str());
    LuaMan->RunFunction();
}

void SetupWheelLua(LuaManager *Man) {
    using namespace rd;
    lua_State *L = Man->GetState();
    luabridge::getGlobalNamespace(L)
            .beginClass<SongWheel>("SongWheel")
            .addFunction("NextDifficulty", &SongWheel::NextDifficulty)
            .addFunction("PrevDifficulty", &SongWheel::PrevDifficulty)
            .addProperty("DifficultyIndex", &SongWheel::GetDifficulty, &SongWheel::SetDifficulty)
            .addFunction("IsLoading", &SongWheel::IsLoading)
            .addFunction("GetIndexAtPoint", &SongWheel::IndexAtPoint)
            .addFunction("GetNormalizedIndexAtPoint", &SongWheel::NormalizedIndexAtPoint)
            .addFunction("GoUp", &SongWheel::GoUp)
            .addFunction("AddSprite", &SongWheel::AddSprite)
            .addFunction("AddString", &SongWheel::AddText)
            .addFunction("ConfirmSelection", &SongWheel::ConfirmSelection)
            .addFunction("IsItemDirectory", &SongWheel::IsItemDirectory)
            .addProperty("SelectedIndex", &SongWheel::GetSelectedItem, &SongWheel::SetSelectedItem)
            .addProperty("CursorIndex", &SongWheel::GetCursorIndex, &SongWheel::SetCursorIndex)
            .addProperty("ListIndex", &SongWheel::GetListCursorIndex)
            .addProperty("ItemCount", &SongWheel::GetNumItems)
            .addData("DisplayStartIndex", &SongWheel::DisplayStartIndex)
            .addData("DisplayItemCount", &SongWheel::DisplayItemCount)
            .endClass();

    luabridge::push(L, &SongWheel::GetInstance());
    lua_setglobal(L, "Wheel");
}


ScreenSelectMusic::ScreenSelectMusic() : Screen("ScreenSelectMusic") {
    PreviewStream = nullptr;

    PreviousPreview = std::make_shared<rd::Song>();
    ToPreview = nullptr;

    SongWheel *Wheel = &SongWheel::GetInstance();
    Wheel->Initialize(GameState::GetInstance().GetSongDatabase());

    SongNotification SongNotifyFunc([this](auto &&PH1, auto &&PH2) {
        OnSongChange(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
    SongNotification SongNotifySelectFunc([this](auto &&PH1, auto &&PH2) {
        OnSongSelect(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
    Wheel->OnSongTentativeSelect = SongNotifyFunc;
    Wheel->OnSongConfirm = SongNotifySelectFunc;

    ListTransformFunction TransformHFunc([this](float &&PH1) -> float { return GetListHorizontalTransformation(PH1); });
    ListTransformFunction TransformVFunc([this](float &&PH1) -> float { return GetListVerticalTransformation(PH1); });
    ListTransformFunction TransformWFunc([this](float &&PH1) -> float { return GetListWidthTransformation(PH1); });
    ListTransformFunction TransformHeightFunc(
            [this](float &&PH1) -> float { return GetListHeightTransformation(PH1); });
    Wheel->TransformHorizontal = TransformHFunc;
    Wheel->TransformVertical = TransformVFunc;
    Wheel->TransformWidth = TransformWFunc;
    Wheel->TransformHeight = TransformHeightFunc;

    DirectoryChangeNotifyFunction DirChangeNotif([this] { OnDirectoryChange(); });
    Wheel->OnDirectoryChange = DirChangeNotif;

    ItemNotification ItClickNotif([this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4) {
        OnItemClick(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                    std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
    });
    ItemNotification ItHoverNotif([this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4) {
        OnItemHover(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                    std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
    });
    ItemNotification ItHoverLeaveNotif([this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4) {
        OnItemHoverLeave(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                         std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
    });
    Wheel->OnItemClick = ItClickNotif;
    Wheel->OnItemHover = ItHoverNotif;
    Wheel->OnItemHoverLeave = ItHoverLeaveNotif;

    Wheel->TransformItem = [this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4) {
        TransformItem(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                      std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
    };
    Wheel->TransformString = [this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4, auto &&PH5) {
        TransformString(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                        std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4),
                        std::forward<decltype(PH5)>(PH5));
    };

    SelectSnd = std::make_unique<AudioSample>();
    SelectSnd->Open(Configuration::GetSkinSound("SongSelectDecision"));

    ClickSnd = std::make_unique<AudioSample>();
    ClickSnd->Open(Configuration::GetSkinSound("SongSelectHover"));

    // rd::dotcur::GameObject::GlobalInit();

    IsTransitioning = false;
    TransitionTime = 0;
}

void ScreenSelectMusic::InitializeResources() {
    LuaManager *LuaM = Animations->GetEnv();

    Animations->InitializeUI();

    Animations->Initialize();

    GameState::GetInstance().InitializeLua(Animations->GetEnv()->GetState());
}

void ScreenSelectMusic::LoadResources() {
    Running = true;

    SwitchBackGuiPending = true;

    SetupWheelLua(Animations->GetEnv());
    Animations->Preload(GameState::GetInstance().GetSkinFile("screenselectmusic.lua"), "Preload");

    Time = 0;
}

void ScreenSelectMusic::Cleanup() {
    if (PreviewStream)
        PreviewStream = nullptr;

    StopLoops();

    SongWheel::GetInstance().CleanItems();
}

float ScreenSelectMusic::GetTransform(const char *TransformName, const float Y) {
    LuaManager *Lua = Animations->GetEnv();
    if (Lua->CallFunction(TransformName, 1, 1)) {
        Lua->PushArgument(Y);
        Lua->RunFunction();
        return Lua->GetFunctionResultF();
    } else return 0;
}

float ScreenSelectMusic::GetListVerticalTransformation(const float Y) {
    return GetTransform("TransformListVertical", Y);
}

float ScreenSelectMusic::GetListHorizontalTransformation(const float Y) {
    return GetTransform("TransformListHorizontal", Y);
}

float ScreenSelectMusic::GetListWidthTransformation(const float Y) {
    return GetTransform("TransformListWidth", Y);
}

float ScreenSelectMusic::GetListHeightTransformation(const float Y) {
    return GetTransform("TransformListHeight", Y);
}

void ScreenSelectMusic::StartGameplayScreen() {
    std::shared_ptr<ScreenLoading> LoadNext;
    std::shared_ptr<rd::Song> MySong = GameState::GetInstance().GetSelectedSongShared();
    auto difindex = SongWheel::GetInstance().GetDifficulty();


    auto VSRGGame = std::make_shared<ScreenGameplay>();

    VSRGGame->Init(std::dynamic_pointer_cast<rd::Song>(MySong));

    LoadNext = std::make_shared<ScreenLoading>(VSRGGame);

    LoadNext->Init();
    Next = LoadNext;
}

void ScreenSelectMusic::OnSongSelect(std::shared_ptr<rd::Song> MySong, uint8_t difindex) {
    // Handle a recently selected song

    if (IsTransitioning)
        return;

    if (difindex > MySong->GetDifficultyCount()) return;

    if (PreviewStream) PreviewStream->Stop();

    IsTransitioning = true;

    SelectSnd->Play();

    StopLoops();

    GameState::GetInstance().SetDifficulty(MySong->Difficulties[difindex], 0);

    Animations->DoEvent("OnSelect", 1);
    TransitionTime = Animations->GetEnv()->GetFunctionResultF();

    SwitchBackGuiPending = true;
}

void ScreenSelectMusic::OnSongChange(std::shared_ptr<rd::Song> MySong, uint8_t difindex) {
    ClickSnd->Play();

    if (MySong) {
        Animations->DoEvent("OnSongChange");

        PreviewWaitTime = 1;
    }

    ToPreview = MySong;
}

void ScreenSelectMusic::PlayPreview() {
    // Do the song preview thing.
    SongDatabase *DB = GameState::GetInstance().GetSongDatabase();
    float StartTime;
    std::string PreviewFile;

    if (ToPreview == nullptr) {
        if (PreviewStream != nullptr)
            PreviewStream->Stop();
        return;
    }

    DB->GetPreviewInfo(ToPreview->ID, PreviewFile, StartTime);

    if (PreviewFile.length() > 0) {
        if (PreviewStream) {
            PreviewStream->Stop();
            PreviewStream = nullptr;
        }

        auto previewPath = ToPreview->SongDirectory / PreviewFile;

        // If missing, find alternate preview file
        if (!std::filesystem::exists(previewPath))
            for (const auto& i : std::filesystem::directory_iterator(ToPreview->SongDirectory)) {
                auto extension = i.path().extension();
                if (extension == ".mp3" || extension == ".ogg")
                    previewPath = i.path();
            }

        // Load preview
        if (std::filesystem::exists(previewPath)) {
            PreviewStream = std::make_shared<AudioStream>();
            if (PreviewStream->Open(previewPath)) {
                PreviewStream->Play();
                PreviewStream->SeekTime(StartTime);
                PreviewStream->SetLoop(true);
            }
        }
    } else {
        if (PreviewStream) {
            PreviewStream->Stop();
            PreviewStream = nullptr;
        }
    }

    PreviousPreview = ToPreview;
}

void ScreenSelectMusic::PlayLoops() {
    if (!BGM) {
        auto fn = Configuration::GetSkinSound("SongSelectBGM");
        BGM = std::make_unique<AudioStream>();
        GetMixer()->AddStream(BGM.get());
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

bool ScreenSelectMusic::Run(double Delta) {
    if (IsTransitioning) {
        if (PreviewStream && PreviewStream->IsPlaying())
            PreviewStream->Stop();

        if (TransitionTime < 0) {
            if (RunNested(Delta))
                return true;
            else {
                IsTransitioning = false;
            }
        } else {
            // We're going to cross the threshold. Fire up the next screen.
            if (TransitionTime - Delta <= 0)
                StartGameplayScreen();

            TransitionTime -= Delta;
        }
    } else {
        if (SwitchBackGuiPending) {
            SwitchBackGuiPending = false;
            PlayLoops();
            Animations->DoEvent("OnRestore");
        }

        PreviewWaitTime -= Delta;
        if (PreviewWaitTime <= 0) {
            if (PreviousPreview != ToPreview)
                PlayPreview();

            if (PreviewStream && PreviewStream->IsPlaying())
                StopLoops();
            else {
                if (!SwitchBackGuiPending)
                    PlayLoops();
            }
        }
    }

    Time += Delta;

    SongWheel::GetInstance().Update(Delta);

    Animations->UpdateTargets(Delta);

    Animations->DrawUntilLayer(16);

    SongWheel::GetInstance().Render();

    Animations->DrawFromLayer(16);

    return Running;
}

void ScreenSelectMusic::StopLoops() {
    if (BGM) {
        BGM->Stop();
        GetMixer()->RemoveStream(BGM.get());
        BGM = nullptr;
    }
}

bool ScreenSelectMusic::HandleInput(int32_t key, bool isPressed, bool isMouseInput) {
    if (TransitionTime > 0 && IsTransitioning)
        return true;

    if (Next)
        return Next->HandleInput(key, isPressed, isMouseInput);


    if (SongWheel::GetInstance().HandleInput(key, isPressed, isMouseInput))
        return true;

    Animations->HandleInput(key, isPressed, isMouseInput);

    if (isPressed) {
        switch (BindingsManager::TranslateKey(key)) {
            case KT_Escape:
                Running = false;
                break;
            case KT_Left:
                SongWheel::GetInstance().PrevDifficulty();
                break;
            case KT_Right:
                SongWheel::GetInstance().NextDifficulty();
                break;
            default:
                break;
        }
    }

    return true;
}

bool ScreenSelectMusic::HandleScrollInput(double xOff, double yOff) {
    if (Next) {
        if (TransitionTime <= 0)
            return Next->HandleScrollInput(xOff, yOff);
        else
            return true;
    }

    if (IsTransitioning) return false;

    Animations->HandleScrollInput(xOff, yOff);
    return SongWheel::GetInstance().HandleScrollInput(xOff, yOff);
}

void ScreenSelectMusic::TransformItem(int Item, std::shared_ptr<rd::Song> Song, bool IsSelected, int Index) {
    if (Animations->GetEnv()->CallFunction("TransformItem", 4)) {
        luabridge::push(Animations->GetEnv()->GetState(), Item);
        luabridge::push(Animations->GetEnv()->GetState(), Song.get());
        luabridge::push(Animations->GetEnv()->GetState(), IsSelected);
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenSelectMusic::TransformString(int Item, std::shared_ptr<rd::Song> Song, bool IsSelected, int Index,
                                        std::string text) {
    if (Animations->GetEnv()->CallFunction("TransformString", 5)) {
        luabridge::push(Animations->GetEnv()->GetState(), Item);
        luabridge::push(Animations->GetEnv()->GetState(), Song.get());
        luabridge::push(Animations->GetEnv()->GetState(), IsSelected);
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), text.c_str());
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenSelectMusic::OnDirectoryChange() {
    Animations->DoEvent("OnDirectoryChange");
}

void ScreenSelectMusic::OnItemClick(int32_t Index, uint32_t boundIndex, std::string Line,
                                    std::shared_ptr<rd::Song> Selected) {
    if (Animations->GetEnv()->CallFunction("OnItemClick", 4)) {
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), boundIndex);
        luabridge::push(Animations->GetEnv()->GetState(), Line);
        luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenSelectMusic::OnItemHover(int32_t Index, uint32_t boundIndex, std::string Line,
                                    std::shared_ptr<rd::Song> Selected) {
    if (Animations->GetEnv()->CallFunction("OnItemHover", 4)) {
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), boundIndex);
        luabridge::push(Animations->GetEnv()->GetState(), Line);
        luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenSelectMusic::OnItemHoverLeave(int32_t Index, uint32_t boundIndex, std::string Line,
                                         std::shared_ptr<rd::Song> Selected) {
    if (Animations->GetEnv()->CallFunction("OnItemHoverLeave", 4)) {
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), boundIndex);
        luabridge::push(Animations->GetEnv()->GetState(), Line);
        luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
        Animations->GetEnv()->RunFunction();
    }
}

