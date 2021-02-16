#include <string>
#include <filesystem>

#include <game/GameConstants.h>
#include <game/ScoreKeeper7K.h>

#include "../LuaManager.h"
#include <LuaBridge/LuaBridge.h>



/// @engineclass ScoreKeeper7K
namespace rd {
    void SetupScorekeeperLuaInterface(void *state) {
        using namespace rd;
        lua_State *L = (lua_State *) state;
        LuaManager helper(L);


#define Constant(x) helper.SetFieldI(#x, x)
        /// Judgments array.
        // @enum Judgment
        // @param SKJ_MISS Miss judgments.
        // @param SKJ_W0 Additional strict judgment.
        // @param SKJ_W1 PGREAT/320/MARVELOUS/COOL
        // @param SKJ_W2 GREAT/300/PERFECT/GOOD
        // @param SKJ_W3 GOOD/200/GREAT/BAD
        // @param SKJ_W4 BAD/100/GOOD
        // @param SKJ_W5 50/BOO
        helper.NewArray();
        Constant(SKJ_MISS);
        Constant(SKJ_W0);
        Constant(SKJ_W1);
        Constant(SKJ_W2);
        Constant(SKJ_W3);
        Constant(SKJ_W4);
        Constant(SKJ_W5);
        helper.FinalizeEnum("Judgment");

        /// Score types
        // @enum ScoreType
        // @param ST_COMBO Combo.
        // @param ST_DP Stepmania DP score
        // @param ST_EX EX score
        // @param ST_EXP Experimental score
        // @param ST_EXP3 Experimenta 3 score
        // @param ST_IIDX IIDX score
        // @param ST_JB Jubeat^2 score
        // @param ST_LR2 LR2 score
        // @param ST_MAX_COMBO Max Combo
        // @param ST_NOTES_HIT Total notes hit
        // @param ST_OSUMANIA osu!mania scoring
        // @param ST_O2JAM O2jam scoring
        helper.NewArray();
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
        helper.FinalizeEnum("ScoreType");

        /// Percentual score types
        // @enum PercentScoreType
        // @param PST_RANK Rank score percentage.
        // @param PST_ACC Raindrop accuracy score.
        // @param PST_EX EX score percentage.
        // @param PST_NH Notes hit percentage.
        // @param PST_OSU osu!mania accuracy percentage.
        helper.NewArray();
        Constant(PST_RANK);
        Constant(PST_ACC);
        Constant(PST_EX);
        Constant(PST_NH);
        Constant(PST_OSU);
        helper.FinalizeEnum("PercentScoreType");

        /// Lifebar types.
        // @enum LifeType
        // @param LT_GROOVE Classic Groove Gauge.
        // @param LT_SURVIVAL Survival gauge.
        // @param LT_EXHARD ExHard gauge.
        // @param LT_DEATH Death gauge.
        // @param LT_EASY Easy gauge.
        // @param LT_O2JAM O2Jam gauge.
        // @param LT_STEPMANIA Stepmania gauge.
        // @param LT_NORECOV No recovery gauge.
        // @param LT_BATTERY Battery gauge.
        helper.NewArray();
        Constant(LT_GROOVE);
        Constant(LT_SURVIVAL);
        Constant(LT_EXHARD);
        Constant(LT_DEATH);
        Constant(LT_EASY);
        Constant(LT_O2JAM);
        Constant(LT_STEPMANIA);
        Constant(LT_NORECOV);
        Constant(LT_BATTERY);
        helper.FinalizeEnum("LifeType");

        /// Pacemaker grade targets.
        // @enum PacemakerType
        // @param PMT_AAA Target AAA.
        // @param PMT_AA Target AA.
        // @param PMT_A Target A.
        // @param PMT_B Target B.
        // @param PMT_C Target C.
        // @param PMT_D Target D.
        // @param PMT_E Target E.
        // @param PMT_F Target F.
        helper.NewArray();
        Constant(PMT_AAA);
        Constant(PMT_AA);
        Constant(PMT_A);
        Constant(PMT_B);
        Constant(PMT_C);
        Constant(PMT_D);
        Constant(PMT_E);
        Constant(PMT_F);
        helper.FinalizeEnum("PacemakerType");

        /// System types.
        // @enum SystemType
        // @param TI_NONE Automatically determine/Not known.
        // @param TI_BMS BMS subsystem.
        // @param TI_OSUMANIA osu!mania subsystem.
        // @param TI_O2JAM O2Jam subsystem.
        // @param TI_STEPMANIA Stepmania subsystem.
        helper.NewArray();
        Constant(TI_NONE);
        Constant(TI_BMS);
        Constant(TI_OSUMANIA);
        Constant(TI_O2JAM);
        Constant(TI_STEPMANIA);
        helper.FinalizeEnum("SystemType");

        /// ScoreKeeper for 7K scores.
        // @type ScoreKeeper7K
        luabridge::getGlobalNamespace(L)
                .beginClass<ScoreKeeper>("ScoreKeeper")
                        /// Max Accuracy achieved (percentual)
                        // @roproperty MaxAccuracy
                .addProperty("MaxAccuracy", &ScoreKeeper::getAccMax)
                        /// Early miss cutoff, in MS
                        // @roproperty EarlyMissCutoffMS
                .addProperty("EarlyMissCutoffMS", &ScoreKeeper::getEarlyMissCutoffMS)
                        /// Whether the W0 window is active
                        // @roproperty UsesW0
                .addProperty("UsesW0", &ScoreKeeper::usesW0)
                        /// The late miss cutoff, in MS.
                        // @roproperty MissCutoffMS
                .addProperty("MissCutoffMS", &ScoreKeeper::getLateMissCutoffMS)
                        /// The current raindrop Rank.
                        // @roproperty Rank
                .addProperty("Rank", &ScoreKeeper::getRank)
                        /// The current Beatmania Rank.
                        // @roproperty BMRank
                .addProperty("BMRank", &ScoreKeeper::getBMRank)
                        /// Whether O2Jam timing is being used.
                        // @roproperty UsesO2
                .addProperty("UsesO2", &ScoreKeeper::usesO2)
                        /// Current O2Jam pill count
                        // @roproperty Pills
                .addProperty("Pills", &ScoreKeeper::getPills)
                        /// Current O2Jam Cool Combo
                        // @roproperty CoolCombo
                .addProperty("CoolCombo", &ScoreKeeper::getCoolCombo)
                        /// Current count of judged notes
                        // @roproperty JudgedNotes
                .addProperty("JudgedNotes", &ScoreKeeper::getTotalNotes)
                        /// Current histogram point count
                        // @roproperty HistogramPointCount
                .addProperty("HistogramPointCount", &ScoreKeeper::getHistogramPointCount)
                        /// Highest point of the histogram
                        // @roproperty HistogramHighestPoint
                .addProperty("HistogramHighestPoint", &ScoreKeeper::getHistogramHighestPoint)
                        /// Average hit off (in MS)
                        // @roproperty AvgHit
                .addProperty("AvgHit", &ScoreKeeper::getAvgHit)
                        /// Standard deviation (in MS)
                        // @roproperty StDev
                .addProperty("StDev", &ScoreKeeper::getHitStDev)
                        /// Max judgable notes
                        // @roproperty MaxNotes
                .addProperty("MaxNotes", &ScoreKeeper::getMaxNotes)
                        /// Confidence offset is wrong in 0-1 range
                        // @roproperty OffsetDistrust
                .addProperty("OffsetDistrust", &ScoreKeeper::getOffsetDistrust)
                        /// Get Judgment window value
                        // @function GetJudgmentWindow
                        // @tparam Judgment judge Judgment to get the window of.
                        // @return The judgment window, in MS
                .addFunction("GetJudgmentWindow", &ScoreKeeper::getJudgmentWindow)
                        /// Get a histogram point
                        // @function GetHistogramPoint
                        // @param index Index of the histogram point
                        // @return The histogram point count at (index - PointCount / 2)ms.
                .addFunction("GetHistogramPoint", &ScoreKeeper::getHistogramPoint)
                        /// Get judgment count
                        // @function GetJudgmentCount
                        // @tparam Judgment judge The judgment to get the count of.
                        // @return The count of the given judgment.
                .addFunction("GetJudgmentCount", &ScoreKeeper::getJudgmentCount)
                        /// Get Score given a type
                        // @function GetScore
                        // @tparam ScoreType type The type of score to get
                        // @return The score of the given parameter type.
                .addFunction("GetScore", &ScoreKeeper::getScore)
                        /// Get a percentual score value
                        // @function GetPercentScore
                        // @tparam PercentScoreType type The percent score type to use.
                        // @return The percentual score, in range 0-100.
                .addFunction("GetPercentScore", &ScoreKeeper::getPercentScore)
                        /// Get whether the stage has been failed given a gauge.
                        // @function IsStageFailed
                        // @tparam LifeType type The gauge type to check for failure.
                        // @treturn bool Whether the stage was failed.
                .addFunction("IsStageFailed", &ScoreKeeper::isStageFailed)
                        /// Whether the current gauge delays failure until the end of the song.
                        // @function hasDelayedFailure
                        // @tparam LifeType type The gauge type to check for whether it ends at the end or not.
                        // @treturn bool Whether the gauge type has a delayed failure.
                .addFunction("HasDelayedFailure", &ScoreKeeper::hasDelayedFailure)
                        /// Get the gauge value given a type.
                        // @function GetLifebarAmount
                        // @tparam LifeType type The gauge type.
                        // @treturn double The value of the gauge, in range 0-1.
                .addFunction("GetLifebarAmount", &ScoreKeeper::getLifebarAmount)
                .endClass();
    }
}

void SetScorekeeperInstance(void *state, rd::ScoreKeeper *Instance) {
    luabridge::push((lua_State *) state, Instance);
    lua_setglobal((lua_State *) state, "ScoreKeeper");
}
