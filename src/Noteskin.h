#ifndef NOTESKIN_H_
#define NOTESKIN_H_

class LuaManager;

class Noteskin
{
	static shared_ptr<LuaManager> NoteskinLua;
public:
	static void SetupNoteskin(bool SpecialStyle, int Lanes);
	static void Update(float Delta, float CurrentBeat);

	static void DrawNote(int Lane, VSRG::TrackNote &T, float Location);
	static void DrawHoldHead(int Lane, float Location);
	static void DrawHoldTail(int Lane, float Location);
	static void DrawHoldBody(int Lane, float Location, float Size);
	static float GetBarlineWidth();
	static double GetBarlineStartX();
	static double GetBarlineOffset();
	static bool IsBarlineEnabled();
	static double GetJudgmentY();
};

#endif