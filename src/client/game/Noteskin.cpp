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

void Noteskin::Validate() {
    /***
     Function called when the Noteskin is created. Called only once.
     @callback Init
     */
    if (NoteskinLua.call_function("Init"))
        NoteskinLua.run_function();
}

int Noteskin::GetChannels() const {
    return Channels;
}

void Noteskin::SetupNoteskin(bool SpecialStyle, int Lanes) {
    CanRender = false;

    Channels = Lanes;

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
    if (!NoteskinLua.run_script(GameState::get_instance().get_skin_file("noteskin.lua"))) {
        Log::LogPrintf("noteskin.lua: %s\n", NoteskinLua.get_last_error().c_str());
    }
}

void Noteskin::update(float Delta, float CurrentBeat) {
    /***
     Update callback. Called every frame.
     @callback Update
     @param delta Time since last frame.
     @param beat Current song beat.
     */
    if (NoteskinLua.call_function("Update", 2)) {
        NoteskinLua.push_argument(Delta);
        NoteskinLua.push_argument(CurrentBeat);
        NoteskinLua.run_function();
    }
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
    if (NoteskinLua.call_function(CallFunc, 4)) {
        NoteskinLua.push_argument(Lane);
        NoteskinLua.push_argument(Location);
        NoteskinLua.push_argument(T.get_frac_kind());
        NoteskinLua.push_argument(0);
        NoteskinLua.run_function();
    }
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

    if (!NoteskinLua.call_function("DrawHoldHead", 4))
        if (!NoteskinLua.call_function("DrawNormal", 4))
            return;

    CanRender = true;
    NoteskinLua.push_argument(Lane);
    NoteskinLua.push_argument(Location);
    NoteskinLua.push_argument(T.get_frac_kind());
    NoteskinLua.push_argument(ActiveLevel);
    NoteskinLua.run_function();
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

    if (!NoteskinLua.call_function("DrawHoldTail", 4))
        if (!NoteskinLua.call_function("DrawNormal", 4))
            return;

    CanRender = true;
    NoteskinLua.push_argument(Lane);
    NoteskinLua.push_argument(Location);
    NoteskinLua.push_argument(T.get_frac_kind());
    NoteskinLua.push_argument(ActiveLevel);
    NoteskinLua.run_function();
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

    if (!NoteskinLua.call_function("DrawHoldBody", 4))
        return;

    CanRender = true;
    NoteskinLua.push_argument(Lane);
    NoteskinLua.push_argument(Location);
    NoteskinLua.push_argument(Size);
    NoteskinLua.push_argument(ActiveLevel);
    NoteskinLua.run_function();
    CanRender = false;
}

		

		