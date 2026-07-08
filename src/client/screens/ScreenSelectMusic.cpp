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
#include "TextureCollection.h"

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

#include "ScreenGameplay.h"

#include "../songdb/SongDatabase.h"
#include "../songdb/SongList.h"
#include "../songdb/SongWheel.h"

#include "../structure/Configuration.h"
#include "../game/Game.h"

void LuaEvt(LuaManager *LuaMan, std::string Func, Sprite *Obj) {
    LuaMan->call_function(Func.c_str());
    LuaMan->run_function();
}

void SetupWheelLua(LuaManager *Man) {
    using namespace rd;
    lua_State *L = Man->get_lua_state();
    luabridge::getGlobalNamespace(L)
            .beginClass<SongWheel>("SongWheel")
            .addFunction("NextDifficulty", &SongWheel::next_difficulty)
            .addFunction("PrevDifficulty", &SongWheel::prev_difficulty)
            .addProperty("DifficultyIndex", &SongWheel::get_difficulty, &SongWheel::set_difficulty)
            .addFunction("IsLoading", &SongWheel::is_loading)
            .addFunction("GetIndexAtPoint", &SongWheel::index_at_point)
            .addFunction("GetNormalizedIndexAtPoint", &SongWheel::normalized_index_at_point)
            .addFunction("GoUp", &SongWheel::go_up)
            .addFunction("AddSprite", &SongWheel::add_sprite)
            .addFunction("AddString", &SongWheel::add_text)
            .addFunction("ConfirmSelection", &SongWheel::confirm_selection)
            .addFunction("IsItemDirectory", &SongWheel::is_item_directory)
            .addProperty("SelectedIndex", &SongWheel::get_selected_item, &SongWheel::set_selected_item)
            .addProperty("CursorIndex", &SongWheel::get_cursor_index, &SongWheel::set_cursor_index)
            .addProperty("ListIndex", &SongWheel::get_list_cursor_index)
            .addProperty("ItemCount", &SongWheel::get_num_items)
            .addData("DisplayStartIndex", &SongWheel::display_start_index)
            .addData("DisplayItemCount", &SongWheel::display_item_count)
            .endClass();

    luabridge::push(L, &SongWheel::get_instance());
    lua_setglobal(L, "Wheel");
}


ScreenSelectMusic::ScreenSelectMusic() : Screen("ScreenSelectMusic") {
    preview_stream_ = nullptr;

    previous_preview = nullptr;
    to_preview = nullptr;

    SongWheel *Wheel = &SongWheel::get_instance();
    Wheel->initialize(GameState::get_instance().get_song_database());

    SongNotification SongNotifyFunc([this](auto &&PH1, auto &&PH2) {
        on_song_change(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
    SongNotification SongNotifySelectFunc([this](auto &&PH1, auto &&PH2) {
        on_song_select(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
    Wheel->on_song_tentative_select = SongNotifyFunc;
    Wheel->on_song_confirm = SongNotifySelectFunc;

    ListTransformFunction transform_h_func([this](float &&PH1) -> float { return get_list_horizontal_transformation(PH1); });
    ListTransformFunction transform_v_func([this](float &&PH1) -> float { return get_list_vertical_transformation(PH1); });
    ListTransformFunction transform_w_func([this](float &&PH1) -> float { return get_list_width_transformation(PH1); });
    ListTransformFunction transform_height_func(
            [this](float &&PH1) -> float { return get_list_height_transformation(PH1); });
    Wheel->transform_horizontal = transform_h_func;
    Wheel->transform_vertical = transform_v_func;
    Wheel->transform_width = transform_w_func;
    Wheel->transform_height = transform_height_func;

    DirectoryChangeNotifyFunction dir_change_notif([this] { on_directory_change(); });
    Wheel->on_directory_change = dir_change_notif;

    ItemNotification it_click_notif([this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4) {
        on_item_click(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                    std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
    });
    ItemNotification it_hover_notif([this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4) {
        on_item_hover(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                    std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
    });
    ItemNotification it_hover_leave_notif([this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4) {
        on_item_hover_leave(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                         std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
    });
    Wheel->on_item_click = it_click_notif;
    Wheel->on_item_hover = it_hover_notif;
    Wheel->on_item_hover_leave = it_hover_leave_notif;

    Wheel->transform_item = [this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4) {
        transform_item(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                      std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4));
    };
    Wheel->transform_string = [this](auto &&PH1, auto &&PH2, auto &&PH3, auto &&PH4, auto &&PH5) {
        transform_string(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                        std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4),
                        std::forward<decltype(PH5)>(PH5));
    };

    select_snd_ = std::make_unique<AudioSample>();
    select_snd_->open(Configuration::GetSkinSound("SongSelectDecision"));

    click_snd_ = std::make_unique<AudioSample>();
    click_snd_->open(Configuration::GetSkinSound("SongSelectHover"));

    // rd::dotcur::GameObject::GlobalInit();

    is_transitioning_ = false;
    transition_time_ = 0;
}

void ScreenSelectMusic::post_load_initialization() {
    auto luam = scene_->get_script_manager();

    scene_->initialize();

    GameState::get_instance().initialize_lua(luam->get_lua_state());
}

void ScreenSelectMusic::load_resources() {
    is_active_ = true;

    switch_back_gui_pending_ = true;

    SetupWheelLua(scene_->get_script_manager());
    scene_->preload(GameState::get_instance().get_skin_file("screenselectmusic.lua"), "Preload");

    time_ = 0;
}

void ScreenSelectMusic::cleanup() {
    if (preview_stream_)
        preview_stream_ = nullptr;

    stop_loops();

    SongWheel::get_instance().clean_items();
}

float ScreenSelectMusic::get_transform(const char *transform_name, const float Y) const {
    if (scene_->call_callback_with_results(transform_name, 1, Y)) {
        return scene_->get_script_manager()->get_stack_f();
    } else return 0;
}

float ScreenSelectMusic::get_list_vertical_transformation(const float Y) const {
    return get_transform("TransformListVertical", Y);
}

float ScreenSelectMusic::get_list_horizontal_transformation(const float Y) const {
    return get_transform("TransformListHorizontal", Y);
}

float ScreenSelectMusic::get_list_width_transformation(const float Y) const {
    return get_transform("TransformListWidth", Y);
}

float ScreenSelectMusic::get_list_height_transformation(const float Y) const {
    return get_transform("TransformListHeight", Y);
}

void ScreenSelectMusic::start_gameplay_screen() {
    auto chart_group = GameState::get_instance().get_selected_chart_group_shared();


    auto screen_gameplay = std::make_shared<ScreenGameplay>();

    screen_gameplay->initialize(chart_group);

    const auto load_next = std::make_shared<ScreenLoading>(screen_gameplay);

    load_next->init();
    next_screen_ = load_next;
}

void ScreenSelectMusic::on_song_select(std::shared_ptr<otoworm::ChartGroup> chart_group, uint8_t difindex) {
    // Handle a recently selected song

    if (is_transitioning_)
        return;

    if (!chart_group || difindex > chart_group->get_chart_count()) return;

    if (preview_stream_) preview_stream_->stop();

    is_transitioning_ = true;

    select_snd_->play();

    stop_loops();

    GameState::get_instance().set_selected_chart_group(chart_group);
    if (difindex < chart_group->charts.size())
        GameState::get_instance().set_chart(chart_group->charts[difindex], 0);

    scene_->trigger_event("OnSelect", 1);
    transition_time_ = scene_->get_script_manager()->get_stack_f();

    switch_back_gui_pending_ = true;
}

void ScreenSelectMusic::on_song_change(std::shared_ptr<otoworm::ChartGroup> chart_group, uint8_t difindex) {
    click_snd_->play();

    if (chart_group) {
        scene_->trigger_event("OnSongChange");
        preview_wait_time_ = 1;
    }

    to_preview = chart_group;
}

void ScreenSelectMusic::play_preview() {
    // Do the song preview thing.
    SongDatabase *DB = GameState::get_instance().get_song_database();
    float start_time;
    std::string preview_file;

    if (to_preview == nullptr) {
        if (preview_stream_ != nullptr)
            preview_stream_->stop();
        return;
    }

    DB->GetPreviewInfo(to_preview->id, preview_file, start_time);

    if (preview_file.length() > 0) {
        if (preview_stream_) {
            preview_stream_->stop();
            preview_stream_ = nullptr;
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
            preview_stream_ = std::make_shared<AudioStream>(get_mixer());
            if (preview_stream_->open(preview_path)) {
                preview_stream_->play();
                preview_stream_->seek_time(start_time);
                preview_stream_->set_loop(true);
            }
        }
    } else {
        if (preview_stream_) {
            preview_stream_->stop();
            preview_stream_ = nullptr;
        }
    }

    previous_preview = to_preview;
}

void ScreenSelectMusic::play_loops() {
    if (!bgm_) {
        auto fn = Configuration::GetSkinSound("SongSelectBGM");
        bgm_ = std::make_unique<AudioStream>(get_mixer());

        if (std::filesystem::exists(fn) &&
            std::filesystem::is_regular_file(fn)) {
            auto s = fn.string();
            auto is_loop = false;
            otoworm::util::to_lower(s);

            if (s.find_first_of("loop") != std::string::npos)
                is_loop = true;

            if (bgm_->open(fn)) {
                bgm_->set_loop(is_loop);
                bgm_->play();
            }
        }
    }
}

bool ScreenSelectMusic::run(const double delta) {
    if (is_transitioning_) {
        if (preview_stream_ && preview_stream_->is_playing())
            preview_stream_->stop();

        if (transition_time_ < 0) {
            if (run_nested(delta))
                return true;
            else {
                is_transitioning_ = false;
            }
        } else {
            // We're going to cross the threshold. Fire up the next screen.
            if (transition_time_ - delta <= 0)
                start_gameplay_screen();

            transition_time_ -= delta;
        }
    } else {
        if (switch_back_gui_pending_) {
            switch_back_gui_pending_ = false;
            play_loops();
            scene_->trigger_event("OnRestore");
        }

        preview_wait_time_ -= delta;
        if (preview_wait_time_ <= 0) {
            if (previous_preview != to_preview)
                play_preview();

            if (preview_stream_ && preview_stream_->is_playing())
                stop_loops();
            else {
                if (!switch_back_gui_pending_)
                    play_loops();
            }
        }
    }

    time_ += delta;

    SongWheel::get_instance().update(delta);

    scene_->update_targets(delta);

    scene_->draw_until_layer(16);

    SongWheel::get_instance().render();

    scene_->draw_from_layer(16);

    return is_active_;
}

void ScreenSelectMusic::stop_loops() {
    if (bgm_) {
        bgm_->stop();
        get_mixer()->remove_stream(bgm_.get());
        bgm_ = nullptr;
    }
}

bool ScreenSelectMusic::on_input(int32_t key, bool isPressed, bool isMouseInput) {
    if (transition_time_ > 0 && is_transitioning_)
        return true;

    if (next_screen_)
        return next_screen_->on_input(key, isPressed, isMouseInput);


    if (SongWheel::get_instance().handle_input(key, isPressed, isMouseInput))
        return true;

    scene_->on_input(key, isPressed, isMouseInput);

    if (isPressed) {
        switch (BindingsManager::translate_key(key)) {
            case KT_Escape:
                is_active_ = false;
                break;
            case KT_Left:
                SongWheel::get_instance().prev_difficulty();
                break;
            case KT_Right:
                SongWheel::get_instance().next_difficulty();
                break;
            default:
                break;
        }
    }

    return true;
}

bool ScreenSelectMusic::on_scroll_input(double xOff, double yOff) {
    if (next_screen_) {
        if (transition_time_ <= 0)
            return next_screen_->on_scroll_input(xOff, yOff);
        else
            return true;
    }

    if (is_transitioning_) return false;

    scene_->on_scroll_input(xOff, yOff);
    return SongWheel::get_instance().handle_scroll_input(xOff, yOff);
}

void ScreenSelectMusic::transform_item(int Item, std::shared_ptr<otoworm::ChartGroup> chart_group, bool IsSelected, int Index) const {
    scene_->call_callback("TransformItem", Item, chart_group.get(), IsSelected, Index);
}

void ScreenSelectMusic::transform_string(int Item, std::shared_ptr<otoworm::ChartGroup> chart_group, bool IsSelected, int Index,
                                        std::string text) const {
    scene_->call_callback("TransformString", Item, chart_group.get(), IsSelected, Index, text);
}

void ScreenSelectMusic::on_directory_change() const {
    scene_->trigger_event("OnDirectoryChange");
}

void ScreenSelectMusic::on_item_click(int32_t Index, uint32_t boundIndex, std::string Line,
                                    std::shared_ptr<otoworm::ChartGroup> Selected) const {
    scene_->call_callback("OnItemClick", Index, boundIndex, Line, Selected.get());
}

void ScreenSelectMusic::on_item_hover(int32_t Index, uint32_t boundIndex, std::string Line,
                                    std::shared_ptr<otoworm::ChartGroup> Selected) const {
    scene_->call_callback("OnItemHover", Index, boundIndex, Line, Selected.get());
}

void ScreenSelectMusic::on_item_hover_leave(int32_t Index, uint32_t boundIndex, std::string Line,
                                         std::shared_ptr<otoworm::ChartGroup> Selected) const {
    scene_->call_callback("OnItemHoverLeave", Index, boundIndex, Line, Selected.get());
}
