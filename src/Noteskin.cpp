#include "GameGlobal.h"
#include "GameState.h"
#include "Sprite.h"
#include "TrackNote.h"
#include "Noteskin.h"
#include "SceneEnvironment.h"

#include "LuaManager.h"
#include <LuaBridge.h>
#include "GameWindow.h"

bool CanRender = false;
shared_ptr<LuaManager> Noteskin::NoteskinLua = nullptr;

void lua_Render(Sprite *S)
{
	if (CanRender)
	{
		Mat4 mt = S->GetMatrix();
		WindowFrame.SetUniform(U_SIM, &mt[0][0]);
		S->RenderMinimalSetup();
	}
}

void Noteskin::SetupNoteskin(bool SpecialStyle, int Lanes)
{
	CanRender = false;
	NoteskinLua = make_shared<LuaManager>();
	DefineSpriteInterface(NoteskinLua.get());

	luabridge::getGlobalNamespace(NoteskinLua->GetState())
		.addFunction("Render", lua_Render);

	NoteskinLua->SetGlobal("SpecialStyle", SpecialStyle);
	NoteskinLua->SetGlobal("Lanes", Lanes);
	NoteskinLua->RunScript(GameState::GetInstance().GetSkinFile("noteskin.lua"));
}

void Noteskin::Update(float Delta)
{
	assert(NoteskinLua != nullptr);

	if (NoteskinLua->CallFunction("Update", 1))
	{
		NoteskinLua->PushArgument(Delta);
		NoteskinLua->RunFunction();
	}
}

void Noteskin::DrawNote(int Lane, VSRG::TrackNote& T, float Location)
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
	if (NoteskinLua->CallFunction(CallFunc, 2))
	{
		NoteskinLua->PushArgument(Lane);
		NoteskinLua->PushArgument(Location);
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

void Noteskin::DrawHoldHead(int Lane, float Location)
{
	assert(NoteskinLua != nullptr);

	if (!NoteskinLua->CallFunction("DrawHoldHead", 2))
		if (!NoteskinLua->CallFunction("DrawNormal", 2))
			return;

	CanRender = true;
	NoteskinLua->PushArgument(Lane);
	NoteskinLua->PushArgument(Location);
	NoteskinLua->RunFunction();
	CanRender = false;
}

void Noteskin::DrawHoldTail(int Lane, float Location)
{
	assert(NoteskinLua != nullptr);

	if (!NoteskinLua->CallFunction("DrawHoldTail", 2))
		if (!NoteskinLua->CallFunction("DrawNormal", 2))
			return;

	CanRender = true;
	NoteskinLua->PushArgument(Lane);
	NoteskinLua->PushArgument(Location);
	NoteskinLua->RunFunction();
	CanRender = false;
}

void Noteskin::DrawHoldBody(int Lane, float Location, float Size)
{
	assert(NoteskinLua != nullptr);

	if (!NoteskinLua->CallFunction("DrawHoldBody", 3))
			return;

	CanRender = true;
	NoteskinLua->PushArgument(Lane);
	NoteskinLua->PushArgument(Location);
	NoteskinLua->PushArgument(Size);
	NoteskinLua->RunFunction();
	CanRender = false;
}