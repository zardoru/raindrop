#include <cstdint>
#include <string>

#include <rmath.h>

#include "PlayscreenParameters.h"
#include "GameState.h"
#include "Logging.h"

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "Noteskin.h"
#include "../structure/SceneEnvironment.h"

#include "GameWindow.h"

#include <game/RaindropProcessedChart.h>
#include <game/VSRGMechanics.h>
#include "PlayerContext.h"

#include "Shader.h"

/// @themescript noteskin.lua
Noteskin::Noteskin(PlayerContext *parent) {
    can_render_ = false;
    note_screen_size_ = 0;
    decrease_hold_size_when_being_hit_ = true;
    dangling_heads_ = true;
    parent_ = parent;

    barline_offset_ = 0;
    barline_enabled_ = false;
    barline_start_x_ = 0;
    barline_width_ = 0;
    judgment_y_ = 0;
}

Noteskin::~Noteskin() = default;

void Noteskin::lua_render(Sprite *s) const {
    if (can_render_ && draw_calls_ && note_shader_ && s)
        s->emit_draw_calls(*draw_calls_, note_shader_.get());
}

bool Noteskin::load_script_callbacks(const std::filesystem::path &filename) {
    auto *state = noteskin_lua_.get_lua_state();

    if (!std::filesystem::exists(filename)) {
        Log::LogPrintf("File %s does not exist\n", filename.string().c_str());
        return false;
    }

    if (luaL_loadfile(state, filename.string().c_str())) {
        const char *reason = lua_tostring(state, -1);
        Log::LogPrintf("noteskin.lua: %s\n", reason ? reason : "unknown error");
        lua_pop(state, 1);
        return false;
    }

    lua_pushcfunction(state, LuaPanic);
    lua_insert(state, -2);

    if (lua_pcall(state, 0, 1, -2)) {
        const char *reason = lua_tostring(state, -1);
        Log::LogPrintf("noteskin.lua: %s\n", reason ? reason : "unknown error");
        lua_pop(state, 1);
        lua_pop(state, 1);
        return false;
    }

    if (lua_istable(state, -1))
        callbacks_.emplace(luabridge::LuaRef::fromStack(state, -1));
    else
        callbacks_.reset();

    lua_pop(state, 1);
    lua_pop(state, 1);
    return true;
}

void Noteskin::log_callback_error(const std::string &name, const std::string &message) const {
    Log::LogPrintf("noteskin callback error in %s: %s\n", name.c_str(), message.c_str());
}

void Noteskin::finalize_loading() {
    /***
     Function called when the Noteskin is created. Called only once.
     @callback Init
     */
    call_callback("Init");
}

int Noteskin::get_channels() const {
    return channels_;
}

void Noteskin::init_noteskin(bool special_style, int lanes) {
    can_render_ = false;

    channels_ = lanes;

    // we need a clean state if we're being called from a different thread (to destroy objects properly)
    DefineSpriteInterface(&noteskin_lua_);

    add_script_classes();

    /// Instance of @{NoteskinObject} provided by the engine.
    // @autoinstance Notes
    luabridge::setGlobal(noteskin_lua_.get_lua_state(), this, "Notes");

    PlayerContext::setup_script_context(&noteskin_lua_);
    /// Instance of @{Player} provided by the engine. Owner of the current noteskin script.
    // @autoinstance Player
    luabridge::setGlobal(noteskin_lua_.get_lua_state(), parent_, "Player");
    load_script_callbacks(GameState::get_instance().get_skin_file("noteskin.lua"));
}

void Noteskin::update(float delta, float current_beat) {
    /***
     Update callback. Called every frame.
     @callback Update
     @param delta Time since last frame.
     @param beat Current song beat.
     */
    call_callback("Update", delta, current_beat);
}

void Noteskin::begin_draw(DrawCallSink &sink) {
    if (!note_shader_) {
        // begin_draw is only reached during rendering, after Shader::Default has compiled its vertex shader.
        note_shader_ = std::make_unique<renderer::Shader::Note>();
    }

    draw_calls_ = &sink;
}
void Noteskin::end_draw() { draw_calls_ = nullptr; }

void Noteskin::set_hidden_effect(const int mode, const float center, const float transition_size,
                                 const float flashlight_size) {
    if (!note_shader_ || !note_shader_->is_valid())
        return;

    note_shader_->set_hidden_effect(mode, center, transition_size, flashlight_size);
}

void Noteskin::draw_note(const rd::RuntimeNote &t, int lane, float location) {
    const char *call_func = nullptr;
    /***
     Draw a normal note.
     @callback DrawNormal
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level Always 0 for normal notes.
     */
    switch (t.get_data_note_kind()) {
        case rd::ENoteKind::NK_NORMAL:
            call_func = "DrawNormal";
            break;
        case rd::ENoteKind::NK_FAKE:
            call_func = "DrawFake";
            break;
        case rd::ENoteKind::NK_INVISIBLE:
            return; // Undrawable
        case rd::ENoteKind::NK_LIFT:
            call_func = "DrawLift";
            break;
        case rd::ENoteKind::NK_MINE:
            call_func = "DrawMine";
            break;
        case rd::ENoteKind::NK_ROLL:
            return; // Unimplemented
    }

    assert(call_func != nullptr);
    // We didn't get a name to call. Odd.

    can_render_ = true;
    call_callback(call_func, lane, location, t.get_frac_kind(), 0);
    can_render_ = false;
}

float Noteskin::get_barline_width() const {
    return barline_width_;
}

double Noteskin::get_barline_start_x() const {
    return barline_start_x_;
}

double Noteskin::get_barline_offset() const {
    return barline_offset_;
}

bool Noteskin::is_barline_enabled() const {
    return barline_enabled_;
}

double Noteskin::get_judgment_y() const {
    return judgment_y_;
}

void Noteskin::draw_hold_head(const rd::RuntimeNote &t, int lane, float location, NoteskinNoteState state) {
    /***
     Draw a hold head. Falls back to DrawNormal if nonexistent
     @callback DrawHoldHead
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level 0 if failed, 1 if active, 2 if being hit, 3 if succesfully hit.
     */

    can_render_ = true;
    const auto active_level = static_cast<int>(state);
    if (!call_callback("DrawHoldHead", lane, location, t.get_frac_kind(), active_level))
        call_callback("DrawNormal", lane, location, t.get_frac_kind(), active_level);
    can_render_ = false;
}

void Noteskin::draw_hold_tail(const rd::RuntimeNote &t, int lane, float location, NoteskinNoteState state) {
    /***
     Draw a hold tail. Falls back to DrawNormal if nonexistent
     @callback DrawHoldTail
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level 0 if failed, 1 if active, 2 if being hit, 3 if succesfully hit.
     */

    can_render_ = true;
    const auto active_level = static_cast<int>(state);
    if (!call_callback("DrawHoldTail", lane, location, t.get_frac_kind(), active_level))
        call_callback("DrawNormal", lane, location, t.get_frac_kind(), active_level);
    can_render_ = false;
}

double Noteskin::get_note_offset() const {
    return note_screen_size_;
}

bool Noteskin::allow_dangling_heads() const {
    return dangling_heads_;
}

bool Noteskin::should_shrink_while_hit() const {
    return decrease_hold_size_when_being_hit_;
}

void Noteskin::draw_hold_body(int lane, float location, float size, NoteskinNoteState state) {
    /***
     Draw a hold body.
     @callback DrawHoldBody
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level 0 if failed, 1 if active, 2 if being hit, 3 if succesfully hit.
     */

    can_render_ = true;
    call_callback("DrawHoldBody", lane, location, size, static_cast<int>(state));
    can_render_ = false;
}

		

		
