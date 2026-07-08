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
    CanRender = false;
    NoteScreenSize = 0;
    DecreaseHoldSizeWhenBeingHit = true;
    DanglingHeads = true;
    Parent = parent;

    BarlineOffset = 0;
    BarlineEnabled = false;
    BarlineStartX = 0;
    BarlineWidth = 0;
    JudgmentY = 0;
}

void Noteskin::LuaRender(Sprite *S) {
    if (CanRender) {
        Mat4 mt = S->GetMatrix();
        renderer::Shader::set_uniform(renderer::DefaultShader::GetUniform(renderer::U_MODELVIEW), &mt[0][0]);
        S->render_minimal_setup();
    }
}

bool Noteskin::load_script_callbacks(const std::filesystem::path &filename) {
    auto *state = NoteskinLua.get_lua_state();

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
        Callbacks.emplace(luabridge::LuaRef::fromStack(state, -1));
    else
        Callbacks.reset();

    lua_pop(state, 1);
    lua_pop(state, 1);
    return true;
}

void Noteskin::log_callback_error(const std::string &name, const std::string &message) const {
    Log::LogPrintf("noteskin callback error in %s: %s\n", name.c_str(), message.c_str());
}

void Noteskin::validate() {
    /***
     Function called when the Noteskin is created. Called only once.
     @callback Init
     */
    call_callback("Init");
}

int Noteskin::get_channels() const {
    return Channels;
}

void Noteskin::init_noteskin(bool special_style, int lanes) {
    CanRender = false;

    Channels = lanes;

    // we need a clean state if we're being called from a different thread (to destroy objects properly)
    DefineSpriteInterface(&NoteskinLua);

    AddScriptClasses();

    /// Instance of @{NoteskinObject} provided by the engine.
    // @autoinstance Notes
    luabridge::setGlobal(NoteskinLua.get_lua_state(), this, "Notes");

    PlayerContext::setup_script_context(&NoteskinLua);
    /// Instance of @{Player} provided by the engine. Owner of the current noteskin script.
    // @autoinstance Player
    luabridge::setGlobal(NoteskinLua.get_lua_state(), Parent, "Player");
    load_script_callbacks(GameState::get_instance().get_skin_file("noteskin.lua"));
}

void Noteskin::update(float Delta, float CurrentBeat) {
    /***
     Update callback. Called every frame.
     @callback Update
     @param delta Time since last frame.
     @param beat Current song beat.
     */
    call_callback("Update", Delta, CurrentBeat);
}

void Noteskin::DrawNote(rd::RuntimeNote &T, int Lane, float Location) {
    const char *CallFunc = nullptr;
    /***
     Draw a normal note.
     @callback DrawNormal
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level Always 0 for normal notes.
     */
    switch (T.get_data_note_kind()) {
        case rd::ENoteKind::NK_NORMAL:
            CallFunc = "DrawNormal";
            break;
        case rd::ENoteKind::NK_FAKE:
            CallFunc = "DrawFake";
            break;
        case rd::ENoteKind::NK_INVISIBLE:
            return; // Undrawable
        case rd::ENoteKind::NK_LIFT:
            CallFunc = "DrawLift";
            break;
        case rd::ENoteKind::NK_MINE:
            CallFunc = "DrawMine";
            break;
        case rd::ENoteKind::NK_ROLL:
            return; // Unimplemented
    }

    assert(CallFunc != nullptr);
    // We didn't get a name to call. Odd.

    CanRender = true;
    call_callback(CallFunc, Lane, Location, T.get_frac_kind(), 0);
    CanRender = false;
}

float Noteskin::GetBarlineWidth() const {
    return BarlineWidth;
}

double Noteskin::GetBarlineStartX() const {
    return BarlineStartX;
}

double Noteskin::GetBarlineOffset() const {
    return BarlineOffset;
}

bool Noteskin::IsBarlineEnabled() const {
    return BarlineEnabled;
}

double Noteskin::GetJudgmentY() const {
    return JudgmentY;
}

void Noteskin::DrawHoldHead(rd::RuntimeNote &T, int Lane, float Location, int ActiveLevel) {
    /***
     Draw a hold head. Falls back to DrawNormal if nonexistent
     @callback DrawHoldHead
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level 0 if failed, 1 if active, 2 if being hit, 3 if succesfully hit.
     */

    CanRender = true;
    if (!call_callback("DrawHoldHead", Lane, Location, T.get_frac_kind(), ActiveLevel))
        call_callback("DrawNormal", Lane, Location, T.get_frac_kind(), ActiveLevel);
    CanRender = false;
}

void Noteskin::DrawHoldTail(rd::RuntimeNote &T, int Lane, float Location, int ActiveLevel) {
    /***
     Draw a hold tail. Falls back to DrawNormal if nonexistent
     @callback DrawHoldTail
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level 0 if failed, 1 if active, 2 if being hit, 3 if succesfully hit.
     */

    CanRender = true;
    if (!call_callback("DrawHoldTail", Lane, Location, T.get_frac_kind(), ActiveLevel))
        call_callback("DrawNormal", Lane, Location, T.get_frac_kind(), ActiveLevel);
    CanRender = false;
}

double Noteskin::GetNoteOffset() const {
    return NoteScreenSize;
}

bool Noteskin::AllowDanglingHeads() const {
    return DanglingHeads;
}

bool Noteskin::ShouldDecreaseHoldSizeWhenBeingHit() const {
    return DecreaseHoldSizeWhenBeingHit;
}

void Noteskin::DrawHoldBody(int Lane, float Location, float Size, int ActiveLevel) {
    /***
     Draw a hold body.
     @callback DrawHoldBody
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level 0 if failed, 1 if active, 2 if being hit, 3 if succesfully hit.
     */

    CanRender = true;
    call_callback("DrawHoldBody", Lane, Location, Size, ActiveLevel);
    CanRender = false;
}

		

		
