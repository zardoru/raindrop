#ifndef NOTESKIN_H_
#define NOTESKIN_H_

class ScreenGameplay7K;
class LuaManager;

class Noteskin
{
	static shared_ptr<LuaManager> NoteskinLua;
	static ScreenGameplay7K* Parent;
	static double NoteScreenSize;
	static bool DanglingHeads;
	static bool DecreaseHoldSizeWhenBeingHit;
public:
	static void Validate();
	static void SetupNoteskin(bool SpecialStyle, int Lanes, ScreenGameplay7K *Parent);
	static void Update(float Delta, float CurrentBeat);
	static void Cleanup();

	static void DrawNote(VSRG::TrackNote &T, int Lane, float Location);
	static void DrawHoldBody(int Lane, float Location, float Size, int ActiveLevel);
	static float GetBarlineWidth();
	static double GetBarlineStartX();
	static double GetBarlineOffset();
	static bool IsBarlineEnabled();
	static double GetJudgmentY();
	static void DrawHoldHead(VSRG::TrackNote& T, int Lane, float Location, int ActiveLevel);
	static void DrawHoldTail(VSRG::TrackNote& T, int Lane, float Location, int ActiveLevel);
	static double GetNoteOffset();
	static bool AllowDanglingHeads();
	static bool ShouldDecreaseHoldSizeWhenBeingHit();
};

#endif