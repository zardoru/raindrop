#include <string>
#include <memory>
#include <filesystem>
#include <thread>
#include <queue>
#include <future>

#include <glm.h>
#include <rmath.h>

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>
#include <text_and_file_util.h>

#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"
#include <ProcessedChart.h>
#include "../game/VSRGMechanics.h"
#include "../game/PlayerContext.h"
#include "Logging.h"
#include "../structure/Screen.h"
#include "../structure/SceneEnvironment.h"
#include "GameWindow.h"
#include "ImageLoader.h"

#include <Audio.h>
#include <sndio/Audiofile.h>
#include <sndio/AudioSourceOJM.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "Line.h"

#include "ScreenSelectMusic.h"
#include "ScreenLoading.h"

#include "../bga/BackgroundAnimation.h"

#include "ScreenGameplay7K.h"

#include "../songdb/SongDatabase.h"
#include "../songdb/SongList.h"
#include "../songdb/SongWheel.h"

#include "../structure/Configuration.h"
#include "../game/Game.h"

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

    previous_preview = nullptr;
    to_preview = nullptr;

    SongWheel *Wheel = &SongWheel::GetInstance();
    Wheel->Initialize(GameState::get_instance().get_song_database());

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
    SelectSnd->open(Configuration::GetSkinSound("SongSelectDecision"));

    ClickSnd = std::make_unique<AudioSample>();
    ClickSnd->open(Configuration::GetSkinSound("SongSelectHover"));

    // rd::dotcur::GameObject::GlobalInit();

    IsTransitioning = false;
    TransitionTime = 0;
}

void ScreenSelectMusic::InitializeResources() {
    auto luam = Animations->GetEnv();

    Animations->Initialize();

    GameState::get_instance().initialize_lua(luam->GetState());
}

void ScreenSelectMusic::LoadResources() {
    Running = true;

    SwitchBackGuiPending = true;

    SetupWheelLua(Animations->GetEnv());
    Animations->Preload(GameState::get_instance().get_skin_file("screenselectmusic.lua"), "Preload");

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
    auto chart_group = GameState::get_instance().get_selected_chart_group_shared();


    auto VSRGGame = std::make_shared<ScreenGameplay>();

    VSRGGame->Init(chart_group);

    LoadNext = std::make_shared<ScreenLoading>(VSRGGame);

    LoadNext->Init();
    Next = LoadNext;
}

void ScreenSelectMusic::OnSongSelect(std::shared_ptr<otoworm::ChartGroup> chart_group, uint8_t difindex) {
    // Handle a recently selected song

    if (IsTransitioning)
        return;

    if (!chart_group || difindex > chart_group->get_chart_count()) return;

    if (PreviewStream) PreviewStream->stop();

    IsTransitioning = true;

    SelectSnd->play();

    StopLoops();

    GameState::get_instance().set_selected_chart_group(chart_group);
    if (difindex < chart_group->charts.size())
        GameState::get_instance().set_chart(chart_group->charts[difindex], 0);

    Animations->DoEvent("OnSelect", 1);
    TransitionTime = Animations->GetEnv()->GetFunctionResultF();

    SwitchBackGuiPending = true;
}

void ScreenSelectMusic::OnSongChange(std::shared_ptr<otoworm::ChartGroup> chart_group, uint8_t difindex) {
    ClickSnd->play();

    if (chart_group) {
        Animations->DoEvent("OnSongChange");

        PreviewWaitTime = 1;
    }

    to_preview = chart_group;
}

void ScreenSelectMusic::PlayPreview() {
    // Do the song preview thing.
    SongDatabase *DB = GameState::get_instance().get_song_database();
    float start_time;
    std::string preview_file;

    if (to_preview == nullptr) {
        if (PreviewStream != nullptr)
            PreviewStream->stop();
        return;
    }

    DB->GetPreviewInfo(to_preview->id, preview_file, start_time);

    if (preview_file.length() > 0) {
        if (PreviewStream) {
            PreviewStream->stop();
            PreviewStream = nullptr;
        }

        auto preview_path = to_preview->path / preview_file;

        // If missing, find alternate preview file
        if (!std::filesystem::exists(preview_path))
            for (const auto& i : std::filesystem::directory_iterator(to_preview->path)) {
                auto extension = i.path().extension();
                if (extension == ".mp3" || extension == ".ogg")
                    preview_path = i.path();
            }

        // Load preview
        if (std::filesystem::exists(preview_path)) {
            PreviewStream = std::make_shared<AudioStream>(GetMixer());
            if (PreviewStream->open(preview_path)) {
                PreviewStream->play();
                PreviewStream->seek_time(start_time);
                PreviewStream->set_loop(true);
            }
        }
    } else {
        if (PreviewStream) {
            PreviewStream->stop();
            PreviewStream = nullptr;
        }
    }

    previous_preview = to_preview;
}

void ScreenSelectMusic::PlayLoops() {
    if (!BGM) {
        auto fn = Configuration::GetSkinSound("SongSelectBGM");
        BGM = std::make_unique<AudioStream>(GetMixer());

        if (std::filesystem::exists(fn) &&
            std::filesystem::is_regular_file(fn)) {
            auto s = fn.string();
            auto IsLoop = false;
            otoworm::util::to_lower(s);

            if (s.find_first_of("loop") != std::string::npos)
                IsLoop = true;

            if (BGM->open(fn)) {
                BGM->set_loop(IsLoop);
                BGM->play();
            }
        }
    }
}

bool ScreenSelectMusic::Run(double Delta) {
    if (IsTransitioning) {
        if (PreviewStream && PreviewStream->is_playing())
            PreviewStream->stop();

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
            if (previous_preview != to_preview)
                PlayPreview();

            if (PreviewStream && PreviewStream->is_playing())
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
        BGM->stop();
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

void ScreenSelectMusic::TransformItem(int Item, std::shared_ptr<otoworm::ChartGroup> chart_group, bool IsSelected, int Index) {
    if (Animations->GetEnv()->CallFunction("TransformItem", 4)) {
        luabridge::push(Animations->GetEnv()->GetState(), Item);
        luabridge::push(Animations->GetEnv()->GetState(), chart_group.get());
        luabridge::push(Animations->GetEnv()->GetState(), IsSelected);
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenSelectMusic::TransformString(int Item, std::shared_ptr<otoworm::ChartGroup> chart_group, bool IsSelected, int Index,
                                        std::string text) {
    if (Animations->GetEnv()->CallFunction("TransformString", 5)) {
        luabridge::push(Animations->GetEnv()->GetState(), Item);
        luabridge::push(Animations->GetEnv()->GetState(), chart_group.get());
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
                                    std::shared_ptr<otoworm::ChartGroup> Selected) {
    if (Animations->GetEnv()->CallFunction("OnItemClick", 4)) {
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), boundIndex);
        luabridge::push(Animations->GetEnv()->GetState(), Line);
        luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenSelectMusic::OnItemHover(int32_t Index, uint32_t boundIndex, std::string Line,
                                    std::shared_ptr<otoworm::ChartGroup> Selected) {
    if (Animations->GetEnv()->CallFunction("OnItemHover", 4)) {
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), boundIndex);
        luabridge::push(Animations->GetEnv()->GetState(), Line);
        luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenSelectMusic::OnItemHoverLeave(int32_t Index, uint32_t boundIndex, std::string Line,
                                         std::shared_ptr<otoworm::ChartGroup> Selected) {
    if (Animations->GetEnv()->CallFunction("OnItemHoverLeave", 4)) {
        luabridge::push(Animations->GetEnv()->GetState(), Index);
        luabridge::push(Animations->GetEnv()->GetState(), boundIndex);
        luabridge::push(Animations->GetEnv()->GetState(), Line);
        luabridge::push(Animations->GetEnv()->GetState(), Selected.get());
        Animations->GetEnv()->RunFunction();
    }
}
