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

namespace Game {
	namespace VSRG {
		Noteskin::Noteskin(PlayerContext *parent) {
			CanRender = false;
			NoteScreenSize = 0;
			DecreaseHoldSizeWhenBeingHit = true;
			DanglingHeads = true;
			Parent = parent;
		}

		void Noteskin::LuaRender(Sprite *S)
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
			if (NoteskinLua.CallFunction("Init"))
				NoteskinLua.RunFunction();

			DecreaseHoldSizeWhenBeingHit = (NoteskinLua.GetGlobalD("DecreaseHoldSizeWhenBeingHit") != 0);
			DanglingHeads = (NoteskinLua.GetGlobalD("DanglingHeads") != 0);
			NoteScreenSize = NoteskinLua.GetGlobalD("NoteScreenSize");
		}

		void Noteskin::SetupNoteskin(bool SpecialStyle, int Lanes)
		{
			CanRender = false;


			// we need a clean state if we're being called from a different thread (to destroy objects properly)
			DefineSpriteInterface(&NoteskinLua);

			luabridge::getGlobalNamespace(NoteskinLua.GetState())
				.beginClass<Noteskin>("Noteskin")
				.addFunction("Render", &Noteskin::LuaRender)
				.endClass();

			NoteskinLua.RunScript(GameState::GetInstance().GetSkinFile("noteskin.lua"));
		}

		void Noteskin::Update(float Delta, float CurrentBeat)
		{
			if (NoteskinLua.CallFunction("Update", 2))
			{
				NoteskinLua.PushArgument(Delta);
				NoteskinLua.PushArgument(CurrentBeat);
				NoteskinLua.RunFunction();
			}
		}

		void Noteskin::DrawNote(TrackNote& T, int Lane, float Location)
		{
			const char* CallFunc = nullptr;

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
			if (NoteskinLua.CallFunction(CallFunc, 4))
			{
				NoteskinLua.PushArgument(Lane);
				NoteskinLua.PushArgument(Location);
				NoteskinLua.PushArgument(T.GetFracKind());
				NoteskinLua.PushArgument(0);
				NoteskinLua.RunFunction();
			}
			CanRender = false;
		}

		float Noteskin::GetBarlineWidth()
		{
			return NoteskinLua.GetGlobalD("BarlineWidth");
		}

		double Noteskin::GetBarlineStartX()
		{
			return NoteskinLua.GetGlobalD("BarlineStartX");
		}

		double Noteskin::GetBarlineOffset()
		{
			return NoteskinLua.GetGlobalD("BarlineOffset");
		}

		bool Noteskin::IsBarlineEnabled()
		{
			return NoteskinLua.GetGlobalD("BarlineEnabled") != 0;
		}

		double Noteskin::GetJudgmentY()
		{
			return NoteskinLua.GetGlobalD("JudgmentLineY");
		}

		void Noteskin::DrawHoldHead(TrackNote &T, int Lane, float Location, int ActiveLevel)
		{
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

		void Noteskin::DrawHoldTail(TrackNote& T, int Lane, float Location, int ActiveLevel)
		{

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
	}
}

		