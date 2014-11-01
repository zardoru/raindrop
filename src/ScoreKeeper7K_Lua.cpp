#include "Global.h"
#include "LuaManager.h"
#include "LuaBridge.h"

#include "ScoreKeeper7K.h"

void SetupScorekeeper7KLuaInterface(void* state)
{
	lua_State* L = (lua_State*)state;

	luabridge::getGlobalNamespace(L)
		.beginClass <ScoreKeeper7K>("ScoreKeeper7K")
		.addFunction("getAccMax", &ScoreKeeper7K::getAccMax)
		.addFunction("getEarlyMissCutoff", &ScoreKeeper7K::getEarlyMissCutoff)
		.addFunction("getJudgmentCount", &ScoreKeeper7K::getJudgmentCount)
		.addFunction("usesW0", &ScoreKeeper7K::usesW0)
		.addFunction("getMissCutoff", &ScoreKeeper7K::getMissCutoff)
		.addFunction("getScore", &ScoreKeeper7K::getScore)
		.addFunction("getPercentScore", &ScoreKeeper7K::getPercentScore)
		.addFunction("getRank", &ScoreKeeper7K::getRank)
		.endClass();

#define cns(x) lua_pushinteger(L,x); lua_setglobal(L, #x);
	cns(SKJ_MISS);
	cns(SKJ_W0);
	cns(SKJ_W1);
	cns(SKJ_W2);
	cns(SKJ_W3);
	cns(SKJ_W4);
	cns(SKJ_W5);
	cns(ST_SCORE);
	cns(ST_COMBO);
	cns(ST_DP);
	cns(ST_EX);
	cns(ST_EXP);
	cns(ST_IIDX);
	cns(ST_JB2);
	cns(ST_LR2);
	cns(ST_MAX_COMBO);
	cns(ST_NOTES_HIT);
	cns(ST_OSUMANIA);
	cns(PST_RANK);
	cns(PST_ACC);
	cns(PST_EX);
	cns(PST_NH);
	cns(PST_OSU);
	cns(LT_GROOVE);
	cns(LT_SURVIVAL);
	cns(LT_EXHARD);
	cns(LT_DEATH);
	cns(LT_EASY);
	cns(LT_STEPMANIA);
	cns(LT_NORECOV);
	cns(LT_BATTERY);
}


void SetScorekeeper7KInstance(void* state, ScoreKeeper7K *Instance)
{
	luabridge::push((lua_State*)state, Instance);
	lua_setglobal((lua_State*)state, "ScoreKeeper");
}