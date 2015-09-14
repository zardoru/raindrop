#include <cmath>
#include <fstream>

#include "GameGlobal.h"
#include "GameState.h"
#include "Screen.h"

#include "LuaManager.h"
#include "SceneEnvironment.h"

#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K.h"

#include "LuaBridge.h"

// Called right after the scorekeeper and the engine's objects are initialized.
void ScreenGameplay7K::SetupScriptConstants()
{
	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("Upscroll", Upscroll);
	L->SetGlobal("Channels", CurrentDiff->Channels);
	L->SetGlobal("JudgmentLineY", JudgmentLinePos);
	L->SetGlobal("Auto", Auto);
	L->SetGlobal("AccuracyHitMS", ScoreKeeper->getMissCutoff());
	L->SetGlobal("SongDuration", CurrentDiff->Duration);
	L->SetGlobal("SongDurationBeats", IntegrateToTime(BPS, CurrentDiff->Duration));
	L->SetGlobal("WaitingTime", WaitingTime);
	L->SetGlobal("Beat", CurrentBeat);
	L->SetGlobal("Lifebar", ScoreKeeper->getLifebarAmount(lifebar_type));
	L->SetGlobal("SpecialStyle", CurrentDiff->Data->Turntable);

	luabridge::push(L->GetState(), &BGA->GetTransformation());
	lua_setglobal(L->GetState(), "Background");
}

// Called every frame, before the lua update event
void ScreenGameplay7K::UpdateScriptVariables()
{
	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("SpeedMultiplier", SpeedMultiplier);
	L->SetGlobal("SpeedMultiplierUser", SpeedMultiplierUser);
	L->SetGlobal("waveEffectEnabled", waveEffectEnabled);
	L->SetGlobal("Active", Active);
	L->SetGlobal("SongTime", SongTime);

	L->SetGlobal("Beat", CurrentBeat);

	L->NewArray();

	for (uint32 i = 0; i < CurrentDiff->Channels; i++)
	{
		L->SetFieldI(i + 1, HeldKey[i]);
	}

	L->FinalizeArray("HeldKeys");

	float CurBPS = SectionValue(BPS, WarpedSongTime);
	L->SetGlobal("CurrentSPB", 1 / CurBPS);
	L->SetGlobal("CurrentBPM", 60 * CurBPS);
}

// Called every time a note is hit or missed, before the lua events
void ScreenGameplay7K::UpdateScriptScoreVariables()
{
	LuaManager *L = Animations->GetEnv();
	L->SetGlobal("Combo", ScoreKeeper->getScore(ST_COMBO));
	L->SetGlobal("MaxCombo", ScoreKeeper->getScore(ST_MAX_COMBO));
	L->SetGlobal("Accuracy", ScoreKeeper->getPercentScore(PST_ACC));
	L->SetGlobal("SCScore", ScoreKeeper->getScore(scoring_type));
	L->SetGlobal("EXScore", ScoreKeeper->getScore(ST_EX));

	std::pair<GString, int> autopacemaker = ScoreKeeper->getAutoRankPacemaker();
	L->SetGlobal("PacemakerText", autopacemaker.first);
	L->SetGlobal("PacemakerValue", autopacemaker.second);

	L->SetGlobal("AccText", "ACC:");
	L->SetGlobal("AccValue", ScoreKeeper->getPercentScore(PST_EX));

	double lifebar_amount = ScoreKeeper->getLifebarAmount(lifebar_type);
	L->SetGlobal("LifebarValue", lifebar_amount);
	if (lifebar_type == LT_GROOVE || lifebar_type == LT_EASY)
		L->SetGlobal("LifebarDisplay", max(2, int(floor(lifebar_amount * 50) * 2)));
	else
		L->SetGlobal("LifebarDisplay", int(ceil(lifebar_amount * 50) * 2));

}

// Called before the script is executed at all.
void ScreenGameplay7K::SetupLua(LuaManager* Env)
{
	GameState::GetInstance().InitializeLua(Env->GetState());
	SetScorekeeper7KInstance(static_cast<void*>(Env->GetState()), ScoreKeeper.get());

#define f(x) addFunction(#x, &ScreenGameplay7K::x)
	luabridge::getGlobalNamespace(Env->GetState())
		.beginClass <ScreenGameplay7K>("ScreenGameplay7K")
		.f(GetSongTime)
		.f(GetCurrentVertical)
		.f(GetCurrentVerticalSpeed)
		.f(GetUserMultiplier)
		.f(GetCurrentBeat)
		.f(IsFailEnabled)
		.f(IsAutoEnabled)
		.f(IsUpscrolling)
		.addProperty("SpeedMultiplier", &ScreenGameplay7K::GetUserMultiplier, &ScreenGameplay7K::SetUserMultiplier);
		
	luabridge::push(Env->GetState(), this);
	lua_setglobal(Env->GetState(), "Game");
}
