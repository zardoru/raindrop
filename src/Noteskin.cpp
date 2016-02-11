#include "pch.h"

#include "GameGlobal.h"
#include "GameState.h"
#include "Logging.h"
#include "Sprite.h"
#include "TrackNote.h"
#include "Noteskin.h"
#include "SceneEnvironment.h"

#include "LuaManager.h"
#include "GameWindow.h"

#include "Screen.h"
#include "ScreenGameplay7K.h"

bool CanRender = false;
std::shared_ptr<LuaManager> Noteskin::NoteskinLua = nullptr;
ScreenGameplay7K* Noteskin::Parent = nullptr;
double Noteskin::NoteScreenSize = 0;
bool Noteskin::DecreaseHoldSizeWhenBeingHit = true;
bool Noteskin::DanglingHeads = true;

void lua_Render(Sprite *S)
{
    if (CanRender)
    {
        Mat4 mt = S->GetMatrix();
        WindowFrame.SetUniform(U_SIM, &mt[0][0]);
        S->RenderMinimalSetup();
    }
}

#ifndef _WIN32
#define LUACHECK(x) if (NoteskinLua == nullptr) {Log::Logf("%s: Noteskin state is invalid - no lua.\n", __func__); return x;}
#else
#define LUACHECK(x) if (NoteskinLua == nullptr) {Log::DebugPrintf("%s: Noteskin state is invalid - no lua.\n", __FUNCTION__); return x;}
#endif

void Noteskin::Validate()
{
    LUACHECK();
    if (NoteskinLua->CallFunction("Init"))
        NoteskinLua->RunFunction();

    DecreaseHoldSizeWhenBeingHit = (NoteskinLua->GetGlobalD("DecreaseHoldSizeWhenBeingHit") != 0);
    DanglingHeads = (NoteskinLua->GetGlobalD("DanglingHeads") != 0);
    NoteScreenSize = NoteskinLua->GetGlobalD("NoteScreenSize");
}

void Noteskin::SetupNoteskin(bool SpecialStyle, int Lanes, ScreenGameplay7K* Parent)
{
    CanRender = false;

    assert(Parent != nullptr);
    Noteskin::Parent = Parent;

    // we need a clean state if we're being called from a different thread (to destroy objects properly)
    assert(NoteskinLua == nullptr);
    NoteskinLua = std::make_shared<LuaManager>();

    Parent->SetupLua(NoteskinLua.get());
    DefineSpriteInterface(NoteskinLua.get());

    luabridge::getGlobalNamespace(NoteskinLua->GetState())
        .addFunction("Render", lua_Render);

    NoteskinLua->SetGlobal("SpecialStyle", SpecialStyle);
    NoteskinLua->SetGlobal("Lanes", Lanes);
    NoteskinLua->RunScript(GameState::GetInstance().GetSkinFile("noteskin.lua"));
}

void Noteskin::Update(float Delta, float CurrentBeat)
{
    LUACHECK();

    if (NoteskinLua->CallFunction("Update", 2))
    {
        NoteskinLua->PushArgument(Delta);
        NoteskinLua->PushArgument(CurrentBeat);
        NoteskinLua->RunFunction();
    }
}

void Noteskin::Cleanup()
{
    NoteskinLua = nullptr;
}

void Noteskin::DrawNote(VSRG::TrackNote& T, int Lane, float Location)
{
    const char* CallFunc = nullptr;

    LUACHECK();

    switch (T.GetDataNoteKind())
    {
    case VSRG::ENoteKind::NK_NORMAL:
        CallFunc = "DrawNormal";
        break;
    case VSRG::ENoteKind::NK_FAKE:
        CallFunc = "DrawFake";
        break;
    case VSRG::ENoteKind::NK_INVISIBLE:
        return; // Undrawable
    case VSRG::ENoteKind::NK_LIFT:
        CallFunc = "DrawLift";
        break;
    case VSRG::ENoteKind::NK_MINE:
        CallFunc = "DrawMine";
        break;
    case VSRG::ENoteKind::NK_ROLL:
        return; // Unimplemented
    }

    assert(CallFunc != nullptr);
    // We didn't get a name to call. Odd.

    CanRender = true;
    if (NoteskinLua->CallFunction(CallFunc, 4))
    {
        NoteskinLua->PushArgument(Lane);
        NoteskinLua->PushArgument(Location);
        NoteskinLua->PushArgument(T.GetFracKind());
        NoteskinLua->PushArgument(0);
        NoteskinLua->RunFunction();
    }
    CanRender = false;
}

float Noteskin::GetBarlineWidth()
{
    LUACHECK(0);
    return NoteskinLua->GetGlobalD("BarlineWidth");
}

double Noteskin::GetBarlineStartX()
{
    LUACHECK(0);
    return NoteskinLua->GetGlobalD("BarlineStartX");
}

double Noteskin::GetBarlineOffset()
{
    LUACHECK(0);
    return NoteskinLua->GetGlobalD("BarlineOffset");
}

bool Noteskin::IsBarlineEnabled()
{
    LUACHECK(false);
    return NoteskinLua->GetGlobalD("BarlineEnabled") != 0;
}

double Noteskin::GetJudgmentY()
{
    LUACHECK(0);
    return NoteskinLua->GetGlobalD("JudgmentLineY");
}

void Noteskin::DrawHoldHead(VSRG::TrackNote &T, int Lane, float Location, int ActiveLevel)
{
    LUACHECK();

    if (!NoteskinLua->CallFunction("DrawHoldHead", 4))
        if (!NoteskinLua->CallFunction("DrawNormal", 4))
            return;

    CanRender = true;
    NoteskinLua->PushArgument(Lane);
    NoteskinLua->PushArgument(Location);
    NoteskinLua->PushArgument(T.GetFracKind());
    NoteskinLua->PushArgument(ActiveLevel);
    NoteskinLua->RunFunction();
    CanRender = false;
}

void Noteskin::DrawHoldTail(VSRG::TrackNote& T, int Lane, float Location, int ActiveLevel)
{
    LUACHECK();

    if (!NoteskinLua->CallFunction("DrawHoldTail", 4))
        if (!NoteskinLua->CallFunction("DrawNormal", 4))
            return;

    CanRender = true;
    NoteskinLua->PushArgument(Lane);
    NoteskinLua->PushArgument(Location);
    NoteskinLua->PushArgument(T.GetFracKind());
    NoteskinLua->PushArgument(ActiveLevel);
    NoteskinLua->RunFunction();
    CanRender = false;
}

double Noteskin::GetNoteOffset()
{
    return NoteScreenSize;
}

bool Noteskin::AllowDanglingHeads()
{
    return DanglingHeads;
}

bool Noteskin::ShouldDecreaseHoldSizeWhenBeingHit()
{
    return DecreaseHoldSizeWhenBeingHit;
}

void Noteskin::DrawHoldBody(int Lane, float Location, float Size, int ActiveLevel)
{
    LUACHECK();

    if (!NoteskinLua->CallFunction("DrawHoldBody", 4))
        return;

    CanRender = true;
    NoteskinLua->PushArgument(Lane);
    NoteskinLua->PushArgument(Location);
    NoteskinLua->PushArgument(Size);
    NoteskinLua->PushArgument(ActiveLevel);
    NoteskinLua->RunFunction();
    CanRender = false;
}