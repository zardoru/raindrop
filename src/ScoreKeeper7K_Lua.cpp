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
				.addProperty("EarlyMissCutoffMS", &ScoreKeeper::getEarlyMissCutoffMS)
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
				.addProperty("OffsetDistrust", &ScoreKeeper::getOffsetDistrust)
				.addFunction("GetJudgmentWindow", &ScoreKeeper::getJudgmentWindow)
				.addFunction("GetHistogramPoint", &ScoreKeeper::getHistogramPoint)
				.addFunction("GetJudgmentCount", &ScoreKeeper::getJudgmentCount)
				.addFunction("GetScore", &ScoreKeeper::getScore)
				.addFunction("GetPercentScore", &ScoreKeeper::getPercentScore)
				.addFunction("IsStageFailed", &ScoreKeeper::isStageFailed)
				.addFunction("HasDelayedFailure", &ScoreKeeper::hasDelayedFailure)
				.addFunction("GetLifebarAmount", &ScoreKeeper::getLifebarAmount)
				.endClass();

#define Constant(x) lua_pushinteger(L,x); lua_setglobal(L, #x);
			Constant(SKJ_MISS);
			Constant(SKJ_W0);
			Constant(SKJ_W1);
			Constant(SKJ_W2);
			Constant(SKJ_W3);
			Constant(SKJ_W4);
			Constant(SKJ_W5);
			Constant(ST_SCORE);
			Constant(ST_COMBO);
			Constant(ST_DP);
			Constant(ST_EX);
			Constant(ST_EXP);
			Constant(ST_EXP3);
			Constant(ST_IIDX);
			Constant(ST_JB2);
			Constant(ST_LR2);
			Constant(ST_MAX_COMBO);
			Constant(ST_NOTES_HIT);
			Constant(ST_OSUMANIA);
			Constant(ST_O2JAM);
			Constant(PST_RANK);
			Constant(PST_ACC);
			Constant(PST_EX);
			Constant(PST_NH);
			Constant(PST_OSU);
			Constant(LT_GROOVE);
			Constant(LT_SURVIVAL);
			Constant(LT_EXHARD);
			Constant(LT_DEATH);
			Constant(LT_EASY);
			Constant(LT_O2JAM);
			Constant(LT_STEPMANIA);
			Constant(LT_NORECOV);
			Constant(LT_BATTERY);
			Constant(PMT_AAA);
			Constant(PMT_AA);
			Constant(PMT_A);
			Constant(PMT_B);
			Constant(PMT_C);
			Constant(PMT_D);
			Constant(PMT_E);
			Constant(PMT_F);
			Constant(TI_NONE);
			Constant(TI_BMS);
			Constant(TI_OSUMANIA);
			Constant(TI_O2JAM);
			Constant(TI_STEPMANIA);
		}
	}
}

void SetScorekeeperInstance(void* state, Game::VSRG::ScoreKeeper *Instance)
{
    luabridge::push((lua_State*)state, Instance);
    lua_setglobal((lua_State*)state, "ScoreKeeper");
}
