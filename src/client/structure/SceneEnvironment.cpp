#include <cstdint>
#include <string>
#include <memory>
#include <filesystem>
#include <map>
#include <functional>

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

bool LuaAnimation(LuaManager *Lua, const std::string &Func, Sprite *Target, const float Frac) {
    if (Lua->call_function(Func.c_str(), 2, 1)) {
        Lua->push_argument(Frac);
        luabridge::push(Lua->get_lua_state(), Target);

        if (Lua->run_function())
            return Lua->get_function_result() > 0;
        else
            return false;
    } else return false;
}

void SceneEnvironment::StopAnimationsForTarget(Sprite *Target) {
    for (auto i = Animations.begin();
         i != Animations.end();
    ) {
        if (i->Target == Target) {
            i = Animations.erase(i);
            if (i == Animations.end()) break;
            else continue;
        }

        i++;
    }
}

void SceneEnvironment::RunIntro(const float Fraction, const float Delta) {
    if (mFrameSkip) {
        mFrameSkip = false;
        return;
    }

    /// Update the screen's intro state
    // @callback UpdateIntro
    // @param fraction The percentage of the intro that is done.
    // @param delta The time passed since last frame.
    if (Lua->call_function("UpdateIntro", 2)) {
        Lua->push_argument(Fraction);
        Lua->push_argument(Delta);
        Lua->run_function();
    }

    draw_from_layer(0);
}

void SceneEnvironment::RunExit(const float Fraction, const float Delta) {
    if (mFrameSkip) {
        mFrameSkip = false;
        return;
    }
    /// Update the screen's transition into the next screen.
    // @callback UpdateExit
    // @param fraction The percentage of the intro that is done.
    // @param delta The time passed since last frame.
    if (Lua->call_function("UpdateExit", 2)) {
        Lua->push_argument(Fraction);
        Lua->push_argument(Delta);
        Lua->run_function();
    }

    draw_from_layer(0);
}

float SceneEnvironment::get_intro_duration() const {
    /// How long the intro section lasts.
    // @modvar IntroDuration
    return std::max(Lua->get_global_d("IntroDuration"), 0.0);
}

float SceneEnvironment::get_exit_duration() const {
    /// How long the outro section lasts.
    // @modvar ExitDuration
    return std::max(Lua->get_global_d("ExitDuration"), 0.0);
}

void SceneEnvironment::add_lua_animation(Sprite *target, const std::string &func_name,
                                       int easing, const float duration, const float delay) {
    Animation Anim;
    Anim.Function = bind(LuaAnimation, Lua.get(), func_name, target, std::placeholders::_1);
    Anim.Easing = (Animation::EEaseType) easing;
    Anim.Duration = duration;
    Anim.Delay = delay;
    Anim.Target = target;

    Animations.push_back(Anim);
}

SceneEnvironment::SceneEnvironment(const char *screen_name, bool init_ui) {
    Animations.reserve(10);
    Lua = std::make_shared<LuaManager>();
    Lua->register_struct("GOMAN", this);


    GameState::get_instance().initialize_lua(Lua->get_lua_state());

    /// Automatic instance of SceneEnvironment for script use.
    // @autoinstance Engine
    CreateLuaInterface(Lua.get());
    Images = std::make_shared<ImageList>(true);
    mFrameSkip = true;

    mScreenName = screen_name;
}

TruetypeFont *SceneEnvironment::create_ttf(const char *Dir) {
    auto *Ret = new TruetypeFont(Dir);
    ManagedFonts.push_back(Ret);
    return Ret;
}

SceneEnvironment::~SceneEnvironment() {
    /// Called when the scene environment will be destroyed.
    // @callback Cleanup
    if (Lua->call_function("Cleanup")) {
        Lua->run_function();
    }

    // Remove all managed drawable objects.
    for (auto i: ManagedObjects)
        delete i;

    for (auto i: ManagedFonts)
        delete i;

    ManagedObjects.clear();
    ManagedFonts.clear();
}

void SceneEnvironment::preload(const std::filesystem::path &Filename, std::string array_name) {
    mInitScript = Filename;

    if (!Lua->run_script(Filename)) {
        Log::LogPrintf("Couldn't run lua script while preloading: %s\n", Lua->get_last_error().c_str());
    }

    if (Lua->use_array(array_name)) {
        Lua->start_iteration();

        while (Lua->iterate_next()) {
            auto s = GameState::get_instance().get_skin_file(Lua->next_g_string());
            Images->AddToList(s, "");
            Lua->pop();
        }

        Lua->pop();
    }
}

void SceneEnvironment::sort() {
    std::ranges::stable_sort(
        Objects,
        [](const Drawable2D *A, const Drawable2D *B) -> bool { return A->GetZ() < B->GetZ(); }
    );
}

Sprite *SceneEnvironment::create_object() {
    auto Out = new Sprite;
    ManagedObjects.push_back(Out);
    add_target(Out, true); // Destroy on reload
    return Out;
}

bool SceneEnvironment::is_managed_object(Drawable2D *Obj) const {
    for (auto i: ManagedObjects) {
        if (Obj == i)
            return true;
    }

    return false;
}

void SceneEnvironment::initialize(const std::filesystem::path &filename, const bool run_script) {
    if (mInitScript.wstring().empty() && !filename.wstring().empty())
        mInitScript = filename;

    if (run_script) {
        if (!Lua->run_script(mInitScript)) {
            Log::LogPrintf("Couldn't load script %s: %s", mInitScript.string().c_str(), Lua->get_last_error().c_str());
        }
    }

    /// This function is called at the initialization phase of the screen.
    // @callback Init
    if (Lua->call_function("Init"))
        Lua->run_function();

    Images->LoadAll();
}

void SceneEnvironment::add_target(Drawable2D *target, const bool is_external) {
    Objects.push_back(target);

    if (is_external)
        ExternalObjects.push_back(target);

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
    lua_State *L = Lua->get_lua_state();
    luabridge::push(L, target);
    lua_setglobal(L, Varname.c_str());
}

void SceneEnvironment::stop_managing_object(Drawable2D *Obj) {
    for (auto i = ManagedObjects.begin(); i != ManagedObjects.end(); ++i) {
        if (Obj == *i) {
            ManagedObjects.erase(i);
            return;
        }
    }
}

void SceneEnvironment::remove_managed_object(Drawable2D *Obj) {
    for (auto i = ManagedObjects.begin(); i != ManagedObjects.end(); ++i) {
        if (*i == Obj) {
            remove_target(*i);
            delete *i;
            ManagedObjects.erase(i);
            return;
        }
    }
}

void SceneEnvironment::on_scroll_input(const double x_off, const double y_off) const {
    /// Called when the mouse scrolls.
    // @callback ScrollEvent
    // @param xoff Change in X scroll.
    // @param yoff Change in Y scroll.
    if (Lua->call_function("ScrollEvent", 2)) {
        Lua->push_argument(x_off);
        Lua->push_argument(y_off);
        Lua->run_function();
    }
}

void SceneEnvironment::remove_managed_objects() {
    for (auto i: ManagedObjects) {
        remove_target(i);
        delete i;
    }

    ManagedObjects.clear();
}

void SceneEnvironment::remove_external_objects() {
    for (auto i: ExternalObjects) {
        remove_target(i);
    }

    ExternalObjects.clear();
}

void SceneEnvironment::remove_target(Drawable2D *target) {
    for (auto i = Objects.begin(); i != Objects.end();) {
        if (*i == target) {
            i = Objects.erase(i);
            continue;
        }

        if (i == Objects.end())
            break;

        ++i;
    }
}

void SceneEnvironment::draw_targets(const double TimeDelta) {
    update_targets(TimeDelta);

    draw_from_layer(0);
}

void SceneEnvironment::update_targets(const double TimeDelta) {
    if (mFrameSkip) {
        mFrameSkip = false;
        return;
    }

    for (auto i = Animations.begin();
         i != Animations.end();) {
        if (i->Delay > 0) {
            i->Delay -= TimeDelta; // Still waiting for this to start.

            if (i->Delay < 0) // We rolled into the negatives.
                i->Time += -i->Delay; // Add it to passed time, to pretend it started right on time.
            else {
                i++;
                continue; // It hasn't began yet, so keep at it.
            }
        } else
            i->Time += TimeDelta;

        if (i->Time >=
            i->Duration) // The animation is done. Call the function one last time with value 1 so it's completed.
        {
            i->Function(1);
            i = Animations.erase(i);
            if (i == Animations.end()) break;
            else continue;
        }

        float frac;

        switch (i->Easing) {
            case Animation::EaseIn:
                frac = pow(i->Time / i->Duration, 2);
                break;
            case Animation::EaseOut:
                frac = i->Time / i->Duration;
                frac = -frac * (frac - 2);
                break;
            case Animation::EaseLinear:
            default:
                frac = i->Time / i->Duration;
        }

        if (!i->Function(frac)) // Says the animation is over?
        {
            i = Animations.erase(i);
            if (i == Animations.end()) break;
            else continue;
        }

        i++;
    }

    /// Main update loop. Called every frame.
    // @callback Update
    // @param delta Change in time since last frame.
    if (Lua->call_function("Update", 1)) {
        Lua->push_argument(TimeDelta);
        Lua->run_function();
    }
}

void SceneEnvironment::ReloadUI() {
}

/* This function right now is broken beyond repair. Don't mind it. */
void SceneEnvironment::reload_scripts() {
    auto InitScript = mInitScript;
    this->~SceneEnvironment();
    new(this) SceneEnvironment(mScreenName.c_str(), false);

    initialize(InitScript);
}

void SceneEnvironment::reload_all() {
    //ReloadUI();
    reload_scripts();
}

void SceneEnvironment::set_screen_name(const std::string &sname) {
    mScreenName = sname;
}

void SceneEnvironment::draw_until_layer(const uint32_t layer) const {
    for (const auto i: Objects) {
        if (i == nullptr) {
            /* throw an error */
            continue;
        }
        if (i->GetZ() <= layer)
            i->render();
    }
}

void SceneEnvironment::draw_from_layer(const uint32_t layer) const {
    for (auto &object: Objects) {
        if (object->GetZ() >= layer)
            object->render();
    }
}

LuaManager *SceneEnvironment::get_script_manager() const {
    return Lua.get();
}

bool SceneEnvironment::on_input(const int32_t key, const bool is_pressed, const bool is_mouse_input) const {
    /// Called when a key is pressed or released.
    // @callback KeyEvent
    // @param key The key code.
    // @param type The type of event. 1 is press, 2 is release.
    // @param isMouseInput Whether this is a mouse button press.
    if (Lua->call_function("KeyEvent", 3)) {
        Lua->push_argument(key);
        Lua->push_argument(is_pressed);
        Lua->push_argument(is_mouse_input);
        Lua->run_function();
    }

    return true;
}

bool SceneEnvironment::handle_text_input(int codepoint) {
    return false;
}

ImageList *SceneEnvironment::get_image_list() const {
    return Images.get();
}

void SceneEnvironment::trigger_event(const std::string &event_name, const int Return) const {
    if (Lua->call_function(event_name.c_str(), 0, Return))
        Lua->run_function();
}

void SceneEnvironment::remove_sprite_target(Sprite *Targ) {
    remove_target(Targ);
}
