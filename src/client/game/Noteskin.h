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
    LuaManager NoteskinLua;
    std::optional<luabridge::LuaRef> Callbacks;
    double NoteScreenSize;
    double BarlineWidth;
    double BarlineStartX;
    double BarlineOffset;
    double JudgmentY;

    int Channels;
    bool BarlineEnabled;
    bool DanglingHeads;
    bool CanRender;
    bool DecreaseHoldSizeWhenBeingHit;
    PlayerContext *Parent;
    DrawCallSink *draw_calls_ = nullptr;

    void LuaRender(Sprite *);

    void AddScriptClasses();
    bool load_script_callbacks(const std::filesystem::path &filename);
    void log_callback_error(const std::string &name, const std::string &message) const;

    template<class... Args>
    bool call_callback(const std::string &event_name, Args&&... args)
    {
        auto *state = NoteskinLua.get_lua_state();

        if (Callbacks && Callbacks->isTable()) {
            auto callback = (*Callbacks)[event_name];
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

        if (NoteskinLua.call_function(event_name.c_str(), sizeof...(Args))) {
            if constexpr (sizeof...(Args) > 0) {
                (luabridge::push(state, std::forward<Args>(args)), ...);
            }
            return NoteskinLua.run_function();
        }

        return false;
    }

public:
    Noteskin(PlayerContext *parent);

    void validate();

    void init_noteskin(bool special_style, int lanes);

    void update(float Delta, float CurrentBeat);
    void begin_draw(DrawCallSink &sink);
    void end_draw();

    void DrawNote(rd::RuntimeNote &T, int Lane, float Location);

    void DrawHoldBody(int Lane, float Location, float Size, int ActiveLevel);

    float GetBarlineWidth() const;

    double GetBarlineStartX() const;

    double GetBarlineOffset() const;

    bool IsBarlineEnabled() const;

    double GetJudgmentY() const;

    void DrawHoldHead(rd::RuntimeNote &T, int Lane, float Location, int ActiveLevel);

    void DrawHoldTail(rd::RuntimeNote &T, int Lane, float Location, int ActiveLevel);

    double GetNoteOffset() const;

    bool AllowDanglingHeads() const;

    bool ShouldDecreaseHoldSizeWhenBeingHit() const;

    int get_channels() const;
};
