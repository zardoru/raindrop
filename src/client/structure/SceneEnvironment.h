#pragma once

#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "LuaManager.h"

#include <LuaBridge/LuaBridge.h>

class Drawable2D;
class Sprite;
class ImageList;
class TruetypeFont;

struct Animation
{
    std::function <bool(float Fraction)> Function;

    float Time, Duration, Delay;
    enum EEaseType
    {
        EaseLinear,
        EaseIn,
        EaseOut
    } Easing;

    Sprite* Target;

    Animation()
    {
        Time = Delay = 0;
        Duration = std::numeric_limits<float>::infinity();
        Target = nullptr;
    }
};

class SceneEnvironment
{
    std::shared_ptr<LuaManager> Lua;
    std::shared_ptr<ImageList> Images;
    std::vector<Drawable2D*> Objects;
    std::vector<Drawable2D*> ManagedObjects;
    std::vector<Drawable2D*> ExternalObjects;
    std::vector<TruetypeFont*> ManagedFonts;
    std::vector <Animation> Animations;
    bool mFrameSkip;
    std::string mScreenName;
    std::filesystem::path mInitScript;
    std::optional<luabridge::LuaRef> mCallbacks;

    bool load_script_callbacks(const std::filesystem::path &filename);
    double get_callback_number(const std::string &name, double default_value) const;
    void log_callback_error(const std::string &name, const std::string &message) const;

public:
    SceneEnvironment(const char* screen_name, bool initGUI = false);
    ~SceneEnvironment();

    void remove_managed_objects();
    void remove_external_objects();

    void reload_scripts();
    void ReloadUI();
    void reload_all();

	void set_screen_name(const std::string &sname);

    void preload(const std::filesystem::path& Filename, std::string array_name);
    void initialize(const std::filesystem::path& filename = "", bool run_script = true);
    LuaManager *get_script_manager() const;
    ImageList* get_image_list() const;

    template<class... Args>
    bool call_callback_with_results(const std::string &event_name, int returns, Args&&... args) const
    {
        auto *state = Lua->get_lua_state();

        if (mCallbacks && mCallbacks->isTable()) {
            auto callback = (*mCallbacks)[event_name];
            if (callback.isFunction()) {
                try {
                    auto result = callback(std::forward<Args>(args)...);
                    if (returns > 0)
                        result.push(state);
                    return true;
                }
                catch (const luabridge::LuaException &e) {
                    log_callback_error(event_name, e.what());
                    return false;
                }
            }
        }

        if (Lua->call_function(event_name.c_str(), sizeof...(Args), returns)) {
            if constexpr (sizeof...(Args) > 0) {
                (luabridge::push(state, std::forward<Args>(args)), ...);
            }
            return Lua->run_function();
        }

        return false;
    }

    template<class... Args>
    bool call_callback(const std::string &event_name, Args&&... args) const
    {
        return call_callback_with_results(event_name, 0, std::forward<Args>(args)...);
    }

    Sprite* create_object();

    void trigger_event(const std::string &event_name, int Return = 0) const;
    void add_lua_animation(Sprite* target, const std::string &FName, int easing, float duration, float delay);
    void StopAnimationsForTarget(Sprite* Target);
    void add_target(Drawable2D* target, bool is_external = false);
    void add_sprite_target(Sprite* Targ);
    void remove_sprite_target(Sprite* Targ);
    void add_lua_target(Sprite *Targ, std::string Varname) const;
    void AddLuaTargetArray(Sprite *Targ, std::string Varname, std::string Arrname);
    void remove_target(Drawable2D *Targ);
    void draw_targets(double TimeDelta);

    TruetypeFont* create_ttf(const char* Dir);

    void sort();

    void update_targets(double TimeDelta);
    void draw_until_layer(uint32_t layer) const;
    void draw_from_layer(uint32_t layer) const;

    void RunIntro(float Fraction, float Delta);
    void RunExit(float Fraction, float Delta);

    float get_intro_duration() const;
    float get_exit_duration() const;

    bool on_input(int32_t key, bool is_pressed, bool is_mouse_input) const;

    static bool handle_text_input(int codepoint);
    bool is_managed_object(Drawable2D *Obj) const;
    void stop_managing_object(Drawable2D *Obj);
    void remove_managed_object(Drawable2D *Obj);
    void on_scroll_input(double x_off, double y_off) const;
};

void DefineSpriteInterface(LuaManager* anim_lua);

void add_rd_lua_global(LuaManager * anim_lua);
