#include <filesystem>

#include <catch.hpp>
#include <LuaManager.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper7K.h>

constexpr auto TIME_RANGE = 10000;



TEST_CASE("Lua Manager state")
{
	LuaManager l;
	REQUIRE_FALSE(l.CallFunction("NonExistingFunction"));
}

using namespace rd;




constexpr auto epsilon = 0.001; // one ms

struct BMSSetup {
	std::shared_ptr<ScoreKeeper> sk;
	RaindropMechanics mech;

	BMSSetup() : mech(true) {
		sk = std::make_shared<ScoreKeeper>();
		sk->setJudgeRank(4); // easy
		mech.Setup(nullptr, sk);
        sk->setTotalObjects(100, 0);

		mech.MissNotify = [&](double t, uint32_t, bool hold, bool nobreakcombo, bool earlymiss) {
            sk->missNote(nobreakcombo, earlymiss, true);
		};
	}
};

struct SMSetup {
	std::shared_ptr<ScoreKeeper> sk;
	RaindropMechanics mech;

	SMSetup() : mech(false) {
		sk = std::make_shared<ScoreKeeper>();
		sk->setSMJ4Windows();
		mech.Setup(nullptr, sk);
        sk->setTotalObjects(100, 0);
	}
};

TEST_CASE("Raindrop Mechanics (general behaviour)", "[general]")
{
	BMSSetup s;

	SECTION("Doesn't act twice on the same note") {
		double lateMissThreshold = s.sk->getLateMissCutoffMS();
		TrackNote t(NoteData{
			0, 0
		});

		REQUIRE(s.mech.OnPressLane(0, &t, 0));
		REQUIRE_FALSE(s.mech.OnPressLane(0, &t, 0));

		t.Reset();
		REQUIRE(s.mech.OnUpdate(lateMissThreshold + epsilon, &t, 0));
		REQUIRE_FALSE(s.mech.OnUpdate(lateMissThreshold + epsilon, &t, 0));

		t.Reset();
		REQUIRE(s.mech.OnUpdate(lateMissThreshold + epsilon, &t, 0));
		REQUIRE_FALSE(s.mech.OnPressLane(0, &t, 0));
	}
}

TEST_CASE("Raindrop Mechanics (BMS tests)", "[raindropbms]") {
	BMSSetup s;

	SECTION("Early misses work properly") {
		double earlyMiss = s.sk->getEarlyMissCutoffMS() / 1000.0;
		TrackNote t(NoteData{
			10, 0
		});

		auto t1 = t.GetStartTime() - earlyMiss + 0.001;
		REQUIRE(s.mech.IsEarlyMiss(t1, &t));
		REQUIRE(s.mech.OnPressLane(t1, &t, 0));

		auto t2 = t.GetStartTime() - earlyMiss + epsilon;
		t.Reset();
		REQUIRE(s.mech.IsEarlyMiss(t2, &t));
		REQUIRE(s.mech.OnPressLane(t2, &t, 0));

		auto t3 = t.GetStartTime() - earlyMiss - epsilon;
		t.Reset();
		REQUIRE_FALSE(s.mech.IsEarlyMiss(t3, &t));
		REQUIRE_FALSE(s.mech.OnPressLane(t3, &t, 0));
	}

	double hitwindow = s.sk->getEarlyHitCutoffMS();
	SECTION("No MISS judgment on the widest window") {
		REQUIRE(s.sk->hitNote(hitwindow, 0, NoteJudgmentPart::NOTE) != SKJ_MISS);
		REQUIRE(s.sk->hitNote(hitwindow, 0, NoteJudgmentPart::NOTE) != SKJ_NONE);
		REQUIRE(s.sk->hitNote(hitwindow - 1, 0, NoteJudgmentPart::NOTE) != SKJ_MISS);
		REQUIRE(s.sk->hitNote(hitwindow - 1, 0, NoteJudgmentPart::NOTE) != SKJ_NONE);
	}

	SECTION("Early Misses don't break combo") {
	    s.sk->init();
	    s.sk->setJudgeRank(4);

	    for (int i = 0; i < 10; i++) {
	        s.sk->hitNote(0, 0, NoteJudgmentPart::NOTE);
	    }

	    REQUIRE(s.sk->getScore(ST_COMBO) == 10);
        REQUIRE(s.sk->hitNote(-s.sk->getEarlyHitCutoffMS() - 1, 0, NoteJudgmentPart::NOTE) == SKJ_MISS);
        REQUIRE(s.sk->getScore(ST_COMBO) == 10);
	}

	SECTION("NONE judgment outside of hit window") {
		double earlymiss = -s.sk->getEarlyMissCutoffMS();
		double latemiss = s.sk->getLateMissCutoffMS();

		REQUIRE(s.sk->hitNote(earlymiss - 1, 0, NoteJudgmentPart::NOTE) == SKJ_NONE);

		// following test is no longer true due to new implementation of timing windows.
		// REQUIRE(s.sk->hitNote(latemiss + 1, 0, NoteJudgmentPart::NOTE) == SKJ_NONE);
	}

	SECTION("Late window misses work as intended") {
		TrackNote t(
			NoteData { 0, 0 }
		);
		double latemiss = s.sk->getLateMissCutoffMS();

		int misses = s.sk->getJudgmentCount(SKJ_MISS);
		REQUIRE(s.mech.OnUpdate(latemiss + epsilon, &t, 0));
		REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses + 1);
	}

	SECTION("No runtime errors across a big range of time") {
	    for (int i = -TIME_RANGE; i <= TIME_RANGE; i++) {
	        double t = (double)i / 1000.0;
            REQUIRE_NOTHROW(s.sk->hitNote(t, 0, NoteJudgmentPart::NOTE));
	    }
	}
}

TEST_CASE("Raindrop Mechanics (Stepmania - LN tails)", "[raindropmechsettails]") {
	SMSetup s;

	SECTION("Tails are not missed earlier than they should") {
		double tailTime = 0.001;
		TrackNote t(NoteData{
			0, tailTime
			});

		// that is inside the judgement area
		REQUIRE_FALSE(s.mech.OnUpdate(0, &t, 0));

		TrackNote t2(NoteData{
			100, 100 + tailTime
			});

		// that is outside the judgment area
		REQUIRE_FALSE(s.mech.OnUpdate(0, &t2, 0));
	}
	
	SECTION("Tails are not missed when the head is still active") {
		double tailTime = 0.001;
		double missCutoff = s.sk->getLateMissCutoffMS() / 1000.0;

		TrackNote t(NoteData{
			0, tailTime
		});

		// Simple tail time < time
		REQUIRE_FALSE(s.mech.OnUpdate(0.06, &t, 0));

		// Tail time < time, head can still be hit, should not miss!
		REQUIRE_FALSE(s.mech.OnUpdate(tailTime + missCutoff - 0.002, &t, 0));
	}

    SECTION("No runtime errors across a big range of time") {
        for (int i = -TIME_RANGE; i <= TIME_RANGE; i++) {
            double t = (double)i / 1000.0;
            REQUIRE_NOTHROW(s.sk->hitNote(t, 0, NoteJudgmentPart::NOTE));
        }
    }
}

