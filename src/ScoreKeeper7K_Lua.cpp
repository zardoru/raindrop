#include "pch.h"

#include "LuaManager.h"
#include "LuaBridge.h"

#include "GameGlobal.h"
#include "ScoreKeeper7K.h"

namespace Game {
	namespace VSRG {
		void SetupScorekeeperLuaInterface(void* state)
		{
			using namespace Game::VSRG;
			lua_State* L = (lua_State*)state;

			luabridge::getGlobalNamespace(L)
				.beginClass <ScoreKeeper>("ScoreKeeper")
				.addProperty("MaxAccuracy", &ScoreKeeper::getAccMax)
				.addProperty("EarlyMissCutoffMS", &ScoreKeeper::getEarlyMissCutoff)
				.addProperty("UsesW0", &ScoreKeeper::usesW0)
				.addProperty("MissCutoffMS", &ScoreKeeper::getMissCutoffMS)
				.addProperty("Rank", &ScoreKeeper::getRank)
				.addProperty("BMRank", &ScoreKeeper::getBMRank)
				.addProperty("UsesO2", &ScoreKeeper::usesO2)
				.addProperty("Pills", &ScoreKeeper::getPills)
				.addProperty("CoolCombo", &ScoreKeeper::getCoolCombo)
				.addProperty("JudgedNotes", &ScoreKeeper::getTotalNotes)
				.addProperty("HistogramPointCount", &ScoreKeeper::getHistogramPointCount)
				.addProperty("HistogramHighestPoint", &ScoreKeeper::getHistogramHighestPoint)
				.addProperty("AvgHit", &ScoreKeeper::getAvgHit)
				.addProperty("StDev", &ScoreKeeper::getHitStDev)
				.addProperty("MaxNotes", &ScoreKeeper::getMaxNotes)
				.addFunction("GetJudgmentWindow", &ScoreKeeper::getJudgmentWindow)
				.addFunction("GetHistogramPoint", &ScoreKeeper::getHistogramPoint)
				.addFunction("GetJudgmentCount", &ScoreKeeper::getJudgmentCount)
				.addFunction("GetScore", &ScoreKeeper::getScore)
				.addFunction("GetPercentScore", &ScoreKeeper::getPercentScore)
				.addFunction("IsStageFailed", &ScoreKeeper::isStageFailed)
				.addFunction("HasDelayedFailure", &ScoreKeeper::hasDelayedFailure)
				.addFunction("GetLifebarAmount", &ScoreKeeper::getLifebarAmount)
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
			cns(ST_EXP3);
			cns(ST_IIDX);
			cns(ST_JB2);
			cns(ST_LR2);
			cns(ST_MAX_COMBO);
			cns(ST_NOTES_HIT);
			cns(ST_OSUMANIA);
			cns(ST_O2JAM);
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
			cns(LT_O2JAM);
			cns(LT_STEPMANIA);
			cns(LT_NORECOV);
			cns(LT_BATTERY);
			cns(PMT_AAA);
			cns(PMT_AA);
			cns(PMT_A);
			cns(PMT_B);
			cns(PMT_C);
			cns(PMT_D);
			cns(PMT_E);
			cns(PMT_F);
			cns(TI_NONE);
			cns(TI_BMS);
			cns(TI_OSUMANIA);
			cns(TI_O2JAM);
			cns(TI_STEPMANIA);
		}
	}
}

void SetScorekeeperInstance(void* state, Game::VSRG::ScoreKeeper *Instance)
{
    luabridge::push((lua_State*)state, Instance);
    lua_setglobal((lua_State*)state, "ScoreKeeper");
}
