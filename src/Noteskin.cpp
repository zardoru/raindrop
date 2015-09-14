#include "GameGlobal.h"
#include "GameState.h"
#include "Sprite.h"
#include "TrackNote.h"
#include "Noteskin.h"
#include "SceneEnvironment.h"

#include "LuaManager.h"
#include <LuaBridge.h>
#include "GameWindow.h"

#include "Screen.h"
#include "ScreenGameplay7K.h"

bool CanRender = false;
shared_ptr<LuaManager> Noteskin::NoteskinLua = nullptr;
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

void Noteskin::Validate()
{
	assert(NoteskinLua != nullptr);
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
	NoteskinLua = make_shared<LuaManager>();

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
	assert(NoteskinLua != nullptr);

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

	assert(NoteskinLua != nullptr);

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
	assert(NoteskinLua != nullptr);
	return NoteskinLua->GetGlobalD("BarlineWidth");
}

double Noteskin::GetBarlineStartX()
{
	assert(NoteskinLua != nullptr);
	return NoteskinLua->GetGlobalD("BarlineStartX");
}

double Noteskin::GetBarlineOffset()
{
	assert(NoteskinLua != nullptr);
	return NoteskinLua->GetGlobalD("BarlineOffset");
}

bool Noteskin::IsBarlineEnabled()
{
	assert(NoteskinLua != nullptr);
	return NoteskinLua->GetGlobalD("BarlineEnabled") != 0;
}

double Noteskin::GetJudgmentY()
{
	assert(NoteskinLua != nullptr);
	return NoteskinLua->GetGlobalD("JudgmentLineY");
}

void Noteskin::DrawHoldHead(VSRG::TrackNote &T, int Lane, float Location, int ActiveLevel)
{
	assert(NoteskinLua != nullptr);

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
	assert(NoteskinLua != nullptr);

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
	assert(NoteskinLua != nullptr);

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