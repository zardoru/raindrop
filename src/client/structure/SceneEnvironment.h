#pragma once

#include <filesystem>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Font.h"
#include "DrawCallSink.h"
#include "LuaManager.h"
#include "Rendering.h"

#include <LuaBridge/LuaBridge.h>

class Drawable2D;
class Sprite;
class ImageList;
class TruetypeFont;
class Texture2D;
class SceneEnvironment
{
    std::shared_ptr<LuaManager> lua_;
    std::shared_ptr<ImageList> images_;
    std::vector<Drawable2D*> objects_;
    std::vector<Drawable2D*> managed_objects_;
    std::vector<Drawable2D*> external_objects_;
    std::vector<TruetypeFont*> managed_fonts_;
    DrawCallSink draw_calls_;
    bool m_frame_skip_;
    std::string m_screen_name_;
    std::filesystem::path m_init_script_;
    std::optional<luabridge::LuaRef> m_callbacks_;

    bool load_script_callbacks(const std::filesystem::path &filename);
    double get_callback_number(const std::string &name, double default_value) const;
    void log_callback_error(const std::string &name, const std::string &message) const;
    void queue_targets();

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
        auto *state = lua_->get_lua_state();

        if (m_callbacks_ && m_callbacks_->isTable()) {
            auto callback = (*m_callbacks_)[event_name];
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

        if (lua_->call_function(event_name.c_str(), sizeof...(Args), returns)) {
            if constexpr (sizeof...(Args) > 0) {
                (luabridge::push(state, std::forward<Args>(args)), ...);
            }
            return lua_->run_function();
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
    void add_target(Drawable2D* target, bool is_external = false);
    void add_sprite_target(Sprite* Targ);
    void remove_sprite_target(Sprite* Targ);
    void add_lua_target(Sprite *Targ, std::string Varname) const;
    void AddLuaTargetArray(Sprite *Targ, std::string Varname, std::string Arrname);
    void remove_target(Drawable2D *Targ);
    void draw_targets(double TimeDelta);
    void draw();
    void draw_quad(uint32_t z, const renderer::QuadDrawParams &params = {});
    void draw_quad(uint32_t z, Texture2D *texture, Transformation *transform,
                   const ColorRGBA &color, int blend_mode);
    void draw_string(uint32_t z, Font *font, std::string text, const Vec2 &position,
                     const Mat4 &transform = Mat4(), const Vec2 &scale = Vec2(1, 1));
    void draw_string(uint32_t z, Font *font, std::string text, const Vec2 &position,
                     float font_size);
    void draw_string(uint32_t z, Font *font, std::string text, const Vec2 &position,
                     float font_size, float kerning_scale = 1.0f);
    void draw_string(uint32_t z, Font *font, std::string text, const Vec2 &position,
                     float font_size, const ColorRGBA &color);
    void draw_string(uint32_t z, Font *font, std::string text, const Vec2 &position,
                     float font_size, const ColorRGBA &color, float kerning_scale);
    DrawCallSink &get_draw_calls() { return draw_calls_; }

    TruetypeFont* create_ttf(const char* Dir);

    void sort();

    void update_targets(double TimeDelta);

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
