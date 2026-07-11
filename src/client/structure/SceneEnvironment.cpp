#include <cstdint>
#include <string>
#include <memory>
#include <filesystem>
#include <map>

#include <rmath.h>

#include <game/GameConstants.h>
#include "../game/Game.h"
#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "Texture2D.h"

#include "SceneEnvironment.h"

#include <algorithm>

#include "ImageList.h"
#include "Logging.h"
#include "GameWindow.h"

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include <SDL3/SDL.h>
#include "TruetypeFont.h"
#include "Configuration.h"
#include "BindingsManager.h"

/// All the other scenes have one of these.
// No @{Object2D} should not be created on the global scope. Only on the callbacks given by the SceneBase.
// Any custom screens will only have what is provided by this module.
/// @themescript SceneBase

void CreateLuaInterface(LuaManager *AnimLua);

bool SceneEnvironment::load_script_callbacks(const std::filesystem::path &filename) {
    auto *state = lua_->get_lua_state();

    if (!std::filesystem::exists(filename)) {
        Log::LogPrintf("File %s does not exist\n", filename.string().c_str());
        return false;
    }

    if (luaL_loadfile(state, filename.string().c_str())) {
        const char *reason = lua_tostring(state, -1);
        Log::LogPrintf("Couldn't load script %s: %s\n", filename.string().c_str(), reason ? reason : "unknown error");
        lua_pop(state, 1);
        return false;
    }

    lua_pushcfunction(state, LuaPanic);
    lua_insert(state, -2);

    if (lua_pcall(state, 0, 1, -2)) {
        const char *reason = lua_tostring(state, -1);
        Log::LogPrintf("Couldn't load script %s: %s\n", filename.string().c_str(), reason ? reason : "unknown error");
        lua_pop(state, 1);
        lua_pop(state, 1);
        return false;
    }

    if (lua_istable(state, -1))
        m_callbacks_.emplace(luabridge::LuaRef::fromStack(state, -1));
    else
        m_callbacks_.reset();

    lua_pop(state, 1);
    lua_pop(state, 1);
    return true;
}

double SceneEnvironment::get_callback_number(const std::string &name, const double default_value) const {
    if (m_callbacks_ && m_callbacks_->isTable()) {
        auto value = (*m_callbacks_)[name];
        if (value.isNumber())
            return value.cast<double>();
    }

    return lua_->get_global_d(name, default_value);
}

void SceneEnvironment::log_callback_error(const std::string &name, const std::string &message) const {
    Log::LogPrintf("lua callback error in %s: %s\n", name.c_str(), message.c_str());
}

void SceneEnvironment::RunIntro(const float Fraction, const float Delta) {
    if (m_frame_skip_) {
        m_frame_skip_ = false;
        return;
    }

    /// Update the screen's intro state
    // @callback UpdateIntro
    // @param fraction The percentage of the intro that is done.
    // @param delta The time passed since last frame.
    call_callback("UpdateIntro", Fraction, Delta);

    draw_calls_.begin_frame();
    queue_targets();
    draw();
}

void SceneEnvironment::RunExit(const float Fraction, const float Delta) {
    if (m_frame_skip_) {
        m_frame_skip_ = false;
        return;
    }
    /// Update the screen's transition into the next screen.
    // @callback UpdateExit
    // @param fraction The percentage of the intro that is done.
    // @param delta The time passed since last frame.
    call_callback("UpdateExit", Fraction, Delta);

    draw_calls_.begin_frame();
    queue_targets();
    draw();
}

float SceneEnvironment::get_intro_duration() const {
    /// How long the intro section lasts.
    // @modvar IntroDuration
    return std::max(get_callback_number("IntroDuration", -1), 0.0);
}

float SceneEnvironment::get_exit_duration() const {
    /// How long the outro section lasts.
    // @modvar ExitDuration
    return std::max(get_callback_number("ExitDuration", -1), 0.0);
}

SceneEnvironment::SceneEnvironment(const char *screen_name, bool init_ui) {
    lua_ = std::make_shared<LuaManager>();
    lua_->register_struct("GOMAN", this);


    GameState::get_instance().initialize_lua(lua_->get_lua_state());

    /// Automatic instance of SceneEnvironment for script use.
    // @autoinstance Engine
    CreateLuaInterface(lua_.get());
    images_ = std::make_shared<ImageList>(true);
    m_frame_skip_ = true;

    m_screen_name_ = screen_name;
}

TruetypeFont *SceneEnvironment::create_ttf(const char *Dir) {
    auto *Ret = new TruetypeFont(Dir);
    managed_fonts_.push_back(Ret);
    return Ret;
}

SceneEnvironment::~SceneEnvironment() {
    /// Called when the scene environment will be destroyed.
    // @callback Cleanup
    call_callback("Cleanup");

    // Remove all managed drawable objects.
    for (auto i: managed_objects_)
        delete i;

    for (auto i: managed_fonts_)
        delete i;

    managed_objects_.clear();
    managed_fonts_.clear();
}

void SceneEnvironment::preload(const std::filesystem::path &Filename, std::string array_name) {
    m_init_script_ = Filename;

    load_script_callbacks(Filename);

    if (m_callbacks_ && m_callbacks_->isTable()) {
        auto preload = (*m_callbacks_)[array_name];
        if (preload.isTable()) {
            for (int i = 1; i <= preload.length(); ++i) {
                auto item = preload[i];
                if (item.isString()) {
                    auto s = GameState::get_instance().get_skin_file(item.cast<std::string>());
                    images_->AddToList(s, "");
                }
            }
            return;
        }
    }

    if (lua_->use_array(array_name)) {
        lua_->start_iteration();

        while (lua_->iterate_next()) {
            auto s = GameState::get_instance().get_skin_file(lua_->next_g_string());
            images_->AddToList(s, "");
            lua_->pop();
        }

        lua_->pop();
    }
}

void SceneEnvironment::sort() {
    std::ranges::stable_sort(
        objects_,
        [](const Drawable2D *A, const Drawable2D *B) -> bool { return A->GetZ() < B->GetZ(); }
    );
}

Sprite *SceneEnvironment::create_object() {
    auto Out = new Sprite;
    managed_objects_.push_back(Out);
    add_target(Out, true); // Destroy on reload
    return Out;
}

bool SceneEnvironment::is_managed_object(Drawable2D *Obj) const {
    for (auto i: managed_objects_) {
        if (Obj == i)
            return true;
    }

    return false;
}

void SceneEnvironment::initialize(const std::filesystem::path &filename, const bool run_script) {
    if (m_init_script_.wstring().empty() && !filename.wstring().empty())
        m_init_script_ = filename;

    if (run_script) {
        load_script_callbacks(m_init_script_);
    }

    /// This function is called at the initialization phase of the screen.
    // @callback Init
    call_callback("Init");

    images_->LoadAll();
}

void SceneEnvironment::add_target(Drawable2D *target, const bool is_external) {
    objects_.push_back(target);

    if (is_external)
        external_objects_.push_back(target);

    sort();
}

void SceneEnvironment::add_sprite_target(Sprite *target) {
    if (target == nullptr) {
        Log::LogPrintf("attempt to add null target\n");
        return;
    }
    add_target(target, false);
}

void SceneEnvironment::add_lua_target(Sprite *target, std::string Varname) const {
    lua_State *L = lua_->get_lua_state();
    luabridge::push(L, target);
    lua_setglobal(L, Varname.c_str());
}

void SceneEnvironment::stop_managing_object(Drawable2D *Obj) {
    for (auto i = managed_objects_.begin(); i != managed_objects_.end(); ++i) {
        if (Obj == *i) {
            managed_objects_.erase(i);
            return;
        }
    }
}

void SceneEnvironment::remove_managed_object(Drawable2D *Obj) {
    for (auto i = managed_objects_.begin(); i != managed_objects_.end(); ++i) {
        if (*i == Obj) {
            remove_target(*i);
            delete *i;
            managed_objects_.erase(i);
            return;
        }
    }
}

void SceneEnvironment::on_scroll_input(const double x_off, const double y_off) const {
    /// Called when the mouse scrolls.
    // @callback ScrollEvent
    // @param xoff Change in X scroll.
    // @param yoff Change in Y scroll.
    call_callback("ScrollEvent", x_off, y_off);
}

void SceneEnvironment::remove_managed_objects() {
    for (auto i: managed_objects_) {
        remove_target(i);
        delete i;
    }

    managed_objects_.clear();
}

void SceneEnvironment::remove_external_objects() {
    for (auto i: external_objects_) {
        remove_target(i);
    }

    external_objects_.clear();
}

void SceneEnvironment::remove_target(Drawable2D *target) {

    for (auto i = objects_.begin(); i != objects_.end();) {
        if (*i == target) {
            i = objects_.erase(i);
            continue;
        }

        if (i == objects_.end())
            break;

        ++i;
    }
}

void SceneEnvironment::draw_targets(const double TimeDelta) {
    update_targets(TimeDelta);
    draw();
}

void SceneEnvironment::queue_targets() {
    for (auto *object : objects_)
        if (object != nullptr) object->emit_draw_calls(draw_calls_);
}

void SceneEnvironment::draw_quad(const uint32_t z, const renderer::QuadDrawParams &params) {
    draw_calls_.submit_quad(z, params, nullptr, false, {});
}

void SceneEnvironment::draw_quad(const uint32_t z, Texture2D *texture, Transformation *transform,
                                 const float red, const float green, const float blue,
                                 const float alpha, const int blend_mode) {
    if (!texture || !transform)
        return;

    auto matrix = transform->GetMatrix();
    renderer::QuadDrawParams params;
    params.model = &matrix;
    params.blend_mode = static_cast<EBlendMode>(blend_mode);
    params.color = {red, green, blue, alpha};
    draw_calls_.submit_quad(z, params, texture, false, {});
}

void SceneEnvironment::draw_string(const uint32_t z, Font *font, std::string text,
                                   const Vec2 &position, const Mat4 &transform, const Vec2 &scale) {
    draw_calls_.submit_string(z, font, std::move(text), position, transform, scale,
                              {1, 1, 1, 1}, 1, false, {});
}

void SceneEnvironment::draw_string(const uint32_t z, Font *font, std::string text,
                                   const Vec2 &position, const float font_size) {
    draw_string(z, font, std::move(text), position, font_size, 1.0f);
}

void SceneEnvironment::draw_string(const uint32_t z, Font *font, std::string text,
                                   const Vec2 &position, const float font_size,
                                   const float kerning_scale) {
    draw_string(z, font, std::move(text), position, Mat4(), Vec2(kerning_scale, font_size));
}

void SceneEnvironment::update_targets(const double TimeDelta) {
    draw_calls_.begin_frame();

    if (m_frame_skip_) {
        m_frame_skip_ = false;
        queue_targets();
        return;
    }

    /// Main update loop. Called every frame.
    // @callback Update
    // @param delta Change in time since last frame.
    call_callback("Update", TimeDelta);
    queue_targets();
}

void SceneEnvironment::ReloadUI() {
}

/* This function right now is broken beyond repair. Don't mind it. */
void SceneEnvironment::reload_scripts() {
    auto InitScript = m_init_script_;
    this->~SceneEnvironment();
    new(this) SceneEnvironment(m_screen_name_.c_str(), false);

    initialize(InitScript);
}

void SceneEnvironment::reload_all() {
    //ReloadUI();
    reload_scripts();
}

void SceneEnvironment::set_screen_name(const std::string &sname) {
    m_screen_name_ = sname;
}

void SceneEnvironment::draw() {
    draw_calls_.flush();
}

LuaManager *SceneEnvironment::get_script_manager() const {
    return lua_.get();
}

bool SceneEnvironment::on_input(const int32_t key, const bool is_pressed, const bool is_mouse_input) const {
    /// Called when a key is pressed or released.
    // @callback KeyEvent
    // @param key The key code.
    // @param type The type of event. 1 is press, 2 is release.
    // @param isMouseInput Whether this is a mouse button press.
    call_callback("KeyEvent", key, is_pressed, is_mouse_input);

    return true;
}

bool SceneEnvironment::handle_text_input(int codepoint) {
    return false;
}

ImageList *SceneEnvironment::get_image_list() const {
    return images_.get();
}

void SceneEnvironment::trigger_event(const std::string &event_name, const int Return) const {
    call_callback_with_results(event_name, Return);
}

void SceneEnvironment::remove_sprite_target(Sprite *Targ) {
    remove_target(Targ);
}
