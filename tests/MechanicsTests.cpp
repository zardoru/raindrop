#include <filesystem>

#include <catch2/catch_test_macros.hpp>
#include <LuaManager.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper.h>

constexpr auto TIME_RANGE = 10000;



TEST_CASE("Lua Manager state")
{
	LuaManager l;
	REQUIRE_FALSE(l.call_function("NonExistingFunction"));
}

using namespace rd;

namespace {
	struct TestRuntimeNote {
		otoworm::TrackNote source;
		RuntimeNoteStorage storage;

		explicit TestRuntimeNote(const otoworm::NoteData& data) : source(data) {
			storage.push_note(source);
		}

		RuntimeNote* get() {
			return storage.note_at(0);
		}
	};
}




constexpr auto epsilon = 0.001; // one ms

struct BMSSetup {
	std::shared_ptr<ScoreKeeper> sk;
	RaindropMechanics mech;

	BMSSetup() : mech(true) {
		sk = std::make_shared<ScoreKeeper>();
		sk->setJudgeRank(4); // easy
		mech.configure(nullptr, sk);
        sk->setTotalObjects(100, 0);

		mech.notify_miss = [&](double t, uint32_t, bool hold, bool nobreakcombo, bool earlymiss) {
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
		mech.configure(nullptr, sk);
        sk->setTotalObjects(100, 0);
	}
};

TEST_CASE("Raindrop Mechanics (general behaviour)", "[general]")
{
	BMSSetup s;

	SECTION("Doesn't act twice on the same note") {
		double lateMissThreshold = s.sk->getLateMissCutoffMS();
		TestRuntimeNote t(otoworm::NoteData{
			0, 0
		});

		REQUIRE(s.mech.OnPressLane(0, t.get(), 0));
		REQUIRE_FALSE(s.mech.OnPressLane(0, t.get(), 0));

		t.get()->reset();
		REQUIRE(s.mech.OnUpdate(lateMissThreshold + epsilon, t.get(), 0));
		REQUIRE_FALSE(s.mech.OnUpdate(lateMissThreshold + epsilon, t.get(), 0));

		t.get()->reset();
		REQUIRE(s.mech.OnUpdate(lateMissThreshold + epsilon, t.get(), 0));
		REQUIRE_FALSE(s.mech.OnPressLane(0, t.get(), 0));
	}
}

TEST_CASE("Raindrop Mechanics (BMS tests)", "[raindropbms]") {
	BMSSetup s;

	SECTION("Early misses work properly") {
		double earlyMiss = s.sk->getEarlyMissCutoffMS() / 1000.0;
		TestRuntimeNote t(otoworm::NoteData{
			10, 0
		});

		auto t1 = t.get()->get_start_time() - earlyMiss + 0.001;
		REQUIRE(s.mech.IsEarlyMiss(t1, t.get()));
		REQUIRE(s.mech.OnPressLane(t1, t.get(), 0));

		auto t2 = t.get()->get_start_time() - earlyMiss + epsilon;
		t.get()->reset();
		REQUIRE(s.mech.IsEarlyMiss(t2, t.get()));
		REQUIRE(s.mech.OnPressLane(t2, t.get(), 0));

		auto t3 = t.get()->get_start_time() - earlyMiss - epsilon;
		t.get()->reset();
		REQUIRE_FALSE(s.mech.IsEarlyMiss(t3, t.get()));
		REQUIRE_FALSE(s.mech.OnPressLane(t3, t.get(), 0));
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
		TestRuntimeNote t(
			otoworm::NoteData { 0, 0 }
		);
		double latemiss = s.sk->getLateMissCutoffMS();

		int misses = s.sk->getJudgmentCount(SKJ_MISS);
		REQUIRE(s.mech.OnUpdate(latemiss + epsilon, t.get(), 0));
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
		TestRuntimeNote t(otoworm::NoteData{
			0, tailTime
			});

		// that is inside the judgement area
		REQUIRE_FALSE(s.mech.OnUpdate(0, t.get(), 0));

		TestRuntimeNote t2(otoworm::NoteData{
			100, 100 + tailTime
			});

		// that is outside the judgment area
		REQUIRE_FALSE(s.mech.OnUpdate(0, t2.get(), 0));
	}
	
	SECTION("Tails are not missed when the head is still active") {
		double tailTime = 0.001;
		double missCutoff = s.sk->getLateMissCutoffMS() / 1000.0;

		TestRuntimeNote t(otoworm::NoteData{
			0, tailTime
		});

		// Simple tail time < time
		REQUIRE_FALSE(s.mech.OnUpdate(0.06, t.get(), 0));

		// Tail time < time, head can still be hit, should not miss!
		REQUIRE_FALSE(s.mech.OnUpdate(tailTime + missCutoff - 0.002, t.get(), 0));
	}

    SECTION("No runtime errors across a big range of time") {
        for (int i = -TIME_RANGE; i <= TIME_RANGE; i++) {
            double t = (double)i / 1000.0;
            REQUIRE_NOTHROW(s.sk->hitNote(t, 0, NoteJudgmentPart::NOTE));
        }
    }
}
