#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <game/RaindropProcessedChart.h>
#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

/*
	A noteskin must first be set up, then validated.
	Then it's in a valid state and you can use whatever you want from it.
	Validation must be done on the main thread since it may create geometry.
	Setup can be done whenever.
*/

class PlayerContext;
class DrawCallSink;

class Noteskin {
    LuaManager noteskin_lua_;
    std::optional<luabridge::LuaRef> callbacks_;
    double note_screen_size_;
    double barline_width_;
    double barline_start_x_;
    double barline_offset_;
    double judgment_y_;

    int channels_{};
    bool barline_enabled_;
    bool dangling_heads_;
    bool can_render_;
    bool decrease_hold_size_when_being_hit_;
    PlayerContext *parent_;
    DrawCallSink *draw_calls_ = nullptr;

    void lua_render(Sprite *) const;

    void add_script_classes();
    bool load_script_callbacks(const std::filesystem::path &filename);
    void log_callback_error(const std::string &name, const std::string &message) const;

    template<class... Args>
    bool call_callback(const std::string &event_name, Args&&... args)
    {
        auto *state = noteskin_lua_.get_lua_state();

        if (callbacks_ && callbacks_->isTable()) {
            auto callback = (*callbacks_)[event_name];
            if (callback.isFunction()) {
                try {
                    callback(std::forward<Args>(args)...);
                    return true;
                }
                catch (const luabridge::LuaException &e) {
                    log_callback_error(event_name, e.what());
                    return false;
                }
            }
        }

        if (noteskin_lua_.call_function(event_name.c_str(), sizeof...(Args))) {
            if constexpr (sizeof...(Args) > 0) {
                (luabridge::push(state, std::forward<Args>(args)), ...);
            }
            return noteskin_lua_.run_function();
        }

        return false;
    }

public:
    Noteskin(PlayerContext *parent);

    void finalize_loading();

    void init_noteskin(bool special_style, int lanes);

    void update(float delta, float current_beat);
    void begin_draw(DrawCallSink &sink);
    void end_draw();

    void draw_note(const rd::RuntimeNote &t, int lane, float location);

    void draw_hold_body(int lane, float location, float size, int active_level);

    float get_barline_width() const;

    double get_barline_start_x() const;

    double get_barline_offset() const;

    bool is_barline_enabled() const;

    double get_judgment_y() const;

    void draw_hold_head(const rd::RuntimeNote &t, int lane, float location, int active_level);

    void draw_hold_tail(const rd::RuntimeNote &t, int lane, float location, int active_level);

    double get_note_offset() const;

    bool allow_dangling_heads() const;

    bool should_decrease_hold_size_when_being_hit() const;

    int get_channels() const;
};
