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

void lua_Render(Sprite *S)
{
	if (CanRender)
	{
		Mat4 mt = S->GetMatrix();
		WindowFrame.SetUniform(U_SIM, &mt[0][0]);
		S->RenderMinimalSetup();
	}
}

void Noteskin::SetupNoteskin(bool SpecialStyle, int Lanes, ScreenGameplay7K* Parent)
{
	CanRender = false;

	assert(Parent != nullptr);
	Noteskin::Parent = Parent;

	NoteskinLua = make_shared<LuaManager>();

	Parent->SetupLua(NoteskinLua.get());
	DefineSpriteInterface(NoteskinLua.get());

	luabridge::getGlobalNamespace(NoteskinLua->GetState())
		.addFunction("Render", lua_Render);

	NoteskinLua->SetGlobal("SpecialStyle", SpecialStyle);
	NoteskinLua->SetGlobal("Lanes", Lanes);
	NoteskinLua->RunScript(GameState::GetInstance().GetSkinFile("noteskin.lua"));

	NoteScreenSize = NoteskinLua->GetGlobalD("NoteScreenSize");
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
	if (NoteskinLua->CallFunction(CallFunc, 3))
	{
		NoteskinLua->PushArgument(Lane);
		NoteskinLua->PushArgument(Location);
		NoteskinLua->PushArgument(T.GetFracKind());
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

void Noteskin::DrawHoldHead(VSRG::TrackNote &T, int Lane, float Location)
{
	assert(NoteskinLua != nullptr);

	if (!NoteskinLua->CallFunction("DrawHoldHead", 3))
		if (!NoteskinLua->CallFunction("DrawNormal", 3))
			return;

	CanRender = true;
	NoteskinLua->PushArgument(Lane);
	NoteskinLua->PushArgument(Location);
	NoteskinLua->PushArgument(T.GetFracKind());
	NoteskinLua->RunFunction();
	CanRender = false;
}

void Noteskin::DrawHoldTail(VSRG::TrackNote& T, int Lane, float Location)
{
	assert(NoteskinLua != nullptr);

	if (!NoteskinLua->CallFunction("DrawHoldTail", 3))
		if (!NoteskinLua->CallFunction("DrawNormal", 3))
			return;

	CanRender = true;
	NoteskinLua->PushArgument(Lane);
	NoteskinLua->PushArgument(Location);
	NoteskinLua->PushArgument(T.GetFracKind());
	NoteskinLua->RunFunction();
	CanRender = false;
}

double Noteskin::GetNoteOffset()
{
	return NoteScreenSize;
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