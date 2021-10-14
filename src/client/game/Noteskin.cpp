#include <cstdint>
#include <string>

#include <rmath.h>

#include <game/Song.h>
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

#include <game/PlayerChartState.h>
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
        Renderer::Shader::SetUniform(Renderer::DefaultShader::GetUniform(Renderer::U_MODELVIEW), &mt[0][0]);
        S->RenderMinimalSetup();
    }
}

void Noteskin::Validate() {
    /***
     Function called when the Noteskin is created. Called only once.
     @callback Init
     */
    if (NoteskinLua.CallFunction("Init"))
        NoteskinLua.RunFunction();
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
    luabridge::setGlobal(NoteskinLua.GetState(), this, "Notes");

    PlayerContext::SetupLua(&NoteskinLua);
    /// Instance of @{Player} provided by the engine. Owner of the current noteskin script.
    // @autoinstance Player
    luabridge::setGlobal(NoteskinLua.GetState(), Parent, "Player");
    if (!NoteskinLua.RunScript(GameState::GetInstance().GetSkinFile("noteskin.lua"))) {
        Log::LogPrintf("noteskin.lua: %s\n", NoteskinLua.GetLastError().c_str());
    }
}

void Noteskin::Update(float Delta, float CurrentBeat) {
    /***
     Update callback. Called every frame.
     @callback Update
     @param delta Time since last frame.
     @param beat Current song beat.
     */
    if (NoteskinLua.CallFunction("Update", 2)) {
        NoteskinLua.PushArgument(Delta);
        NoteskinLua.PushArgument(CurrentBeat);
        NoteskinLua.RunFunction();
    }
}

void Noteskin::DrawNote(rd::TrackNote &T, int Lane, float Location) {
    const char *CallFunc = nullptr;
    /***
     Draw a normal note.
     @callback DrawNormal
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level Always 0 for normal notes.
     */
    switch (T.GetDataNoteKind()) {
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
    if (NoteskinLua.CallFunction(CallFunc, 4)) {
        NoteskinLua.PushArgument(Lane);
        NoteskinLua.PushArgument(Location);
        NoteskinLua.PushArgument(T.GetFracKind());
        NoteskinLua.PushArgument(0);
        NoteskinLua.RunFunction();
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

void Noteskin::DrawHoldHead(rd::TrackNote &T, int Lane, float Location, int ActiveLevel) {
    /***
     Draw a hold head. Falls back to DrawNormal if nonexistent
     @callback DrawHoldHead
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level 0 if failed, 1 if active, 2 if being hit, 3 if succesfully hit.
     */

    if (!NoteskinLua.CallFunction("DrawHoldHead", 4))
        if (!NoteskinLua.CallFunction("DrawNormal", 4))
            return;

    CanRender = true;
    NoteskinLua.PushArgument(Lane);
    NoteskinLua.PushArgument(Location);
    NoteskinLua.PushArgument(T.GetFracKind());
    NoteskinLua.PushArgument(ActiveLevel);
    NoteskinLua.RunFunction();
    CanRender = false;
}

void Noteskin::DrawHoldTail(rd::TrackNote &T, int Lane, float Location, int ActiveLevel) {
    /***
     Draw a hold tail. Falls back to DrawNormal if nonexistent
     @callback DrawHoldTail
     @param lane Lane of the note.
     @param loc_y Nominal Y position of the note.
     @param fraction Measure subdivision of this note.
     @param active_level 0 if failed, 1 if active, 2 if being hit, 3 if succesfully hit.
     */

    if (!NoteskinLua.CallFunction("DrawHoldTail", 4))
        if (!NoteskinLua.CallFunction("DrawNormal", 4))
            return;

    CanRender = true;
    NoteskinLua.PushArgument(Lane);
    NoteskinLua.PushArgument(Location);
    NoteskinLua.PushArgument(T.GetFracKind());
    NoteskinLua.PushArgument(ActiveLevel);
    NoteskinLua.RunFunction();
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

    if (!NoteskinLua.CallFunction("DrawHoldBody", 4))
        return;

    CanRender = true;
    NoteskinLua.PushArgument(Lane);
    NoteskinLua.PushArgument(Location);
    NoteskinLua.PushArgument(Size);
    NoteskinLua.PushArgument(ActiveLevel);
    NoteskinLua.RunFunction();
    CanRender = false;
}

		

		