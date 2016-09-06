#pragma once

#include "LuaManager.h"

/*
	A noteskin must first be set up, then validated.
	Then it's in a valid state and you can use whatever you want from it.
	Validation must be done on the main thread since it may create geometry.
	Setup can be done whenever.
*/

namespace Game {
	namespace VSRG {
		class PlayerContext;
			
		class Noteskin
		{
			LuaManager NoteskinLua;
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
			PlayerContext* Parent;
			void LuaRender(Sprite*);

		public:
			Noteskin(PlayerContext *parent);
			void Validate();
			void SetupNoteskin(bool SpecialStyle, int Lanes);
			void Update(float Delta, float CurrentBeat);

			void DrawNote(TrackNote &T, int Lane, float Location);
			void DrawHoldBody(int Lane, float Location, float Size, int ActiveLevel);
			float GetBarlineWidth() const;
			double GetBarlineStartX() const;
			double GetBarlineOffset() const;
			bool IsBarlineEnabled() const;
			double GetJudgmentY() const;
			void DrawHoldHead(TrackNote& T, int Lane, float Location, int ActiveLevel);
			void DrawHoldTail(TrackNote& T, int Lane, float Location, int ActiveLevel);
			double GetNoteOffset() const;
			bool AllowDanglingHeads() const;
			bool ShouldDecreaseHoldSizeWhenBeingHit() const;
			int GetChannels() const;
		};
	}
}