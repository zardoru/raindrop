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
		sk->set_judge_rank(4); // easy
		mech.configure(nullptr, sk);
        sk->set_total_objects(100, 0);

		mech.notify_miss = [&](double t, uint32_t, bool hold, bool nobreakcombo, bool earlymiss) {
            sk->miss_note(nobreakcombo, earlymiss, true);
		};
	}
};

struct SMSetup {
	std::shared_ptr<ScoreKeeper> sk;
	RaindropMechanics mech;

	SMSetup() : mech(false) {
		sk = std::make_shared<ScoreKeeper>();
		sk->set_smj4_windows();
		mech.configure(nullptr, sk);
        sk->set_total_objects(100, 0);
	}
};

TEST_CASE("Raindrop Mechanics (general behaviour)", "[general]")
{
	BMSSetup s;

	SECTION("Doesn't act twice on the same note") {
		double lateMissThreshold = s.sk->get_late_miss_cutoff_ms();
		TestRuntimeNote t(otoworm::NoteData{
			0, 0
		});

		REQUIRE(s.mech.on_press_lane(0, t.get(), 0));
		REQUIRE_FALSE(s.mech.on_press_lane(0, t.get(), 0));

		t.get()->reset();
		REQUIRE(s.mech.on_update(lateMissThreshold + epsilon, t.get(), 0));
		REQUIRE_FALSE(s.mech.on_update(lateMissThreshold + epsilon, t.get(), 0));

		t.get()->reset();
		REQUIRE(s.mech.on_update(lateMissThreshold + epsilon, t.get(), 0));
		REQUIRE_FALSE(s.mech.on_press_lane(0, t.get(), 0));
	}
}

TEST_CASE("Raindrop Mechanics (BMS tests)", "[raindropbms]") {
	BMSSetup s;

	SECTION("Early misses work properly") {
		double earlyMiss = s.sk->get_early_miss_cutoff_ms() / 1000.0;
		TestRuntimeNote t(otoworm::NoteData{
			10, 0
		});

		auto t1 = t.get()->get_start_time() - earlyMiss + 0.001;
		REQUIRE(s.mech.is_early_miss(t1, t.get()));
		REQUIRE(s.mech.on_press_lane(t1, t.get(), 0));

		auto t2 = t.get()->get_start_time() - earlyMiss + epsilon;
		t.get()->reset();
		REQUIRE(s.mech.is_early_miss(t2, t.get()));
		REQUIRE(s.mech.on_press_lane(t2, t.get(), 0));

		auto t3 = t.get()->get_start_time() - earlyMiss - epsilon;
		t.get()->reset();
		REQUIRE_FALSE(s.mech.is_early_miss(t3, t.get()));
		REQUIRE_FALSE(s.mech.on_press_lane(t3, t.get(), 0));
	}

	double hitwindow = s.sk->get_early_hit_cutoff_ms();
	SECTION("No MISS judgment on the widest window") {
		REQUIRE(s.sk->hit_note(hitwindow, 0, NoteJudgmentPart::NOTE) != SKJ_MISS);
		REQUIRE(s.sk->hit_note(hitwindow, 0, NoteJudgmentPart::NOTE) != SKJ_NONE);
		REQUIRE(s.sk->hit_note(hitwindow - 1, 0, NoteJudgmentPart::NOTE) != SKJ_MISS);
		REQUIRE(s.sk->hit_note(hitwindow - 1, 0, NoteJudgmentPart::NOTE) != SKJ_NONE);
	}

	SECTION("Early Misses don't break combo") {
	    s.sk->init();
	    s.sk->set_judge_rank(4);

	    for (int i = 0; i < 10; i++) {
	        s.sk->hit_note(0, 0, NoteJudgmentPart::NOTE);
	    }

	    REQUIRE(s.sk->get_score(ST_COMBO) == 10);
        REQUIRE(s.sk->hit_note(-s.sk->get_early_hit_cutoff_ms() - 1, 0, NoteJudgmentPart::NOTE) == SKJ_MISS);
        REQUIRE(s.sk->get_score(ST_COMBO) == 10);
	}

	SECTION("NONE judgment outside of hit window") {
		double earlymiss = -s.sk->get_early_miss_cutoff_ms();
		double latemiss = s.sk->get_late_miss_cutoff_ms();

		REQUIRE(s.sk->hit_note(earlymiss - 1, 0, NoteJudgmentPart::NOTE) == SKJ_NONE);

		// following test is no longer true due to new implementation of timing windows.
		// REQUIRE(s.sk->hit_note(latemiss + 1, 0, NoteJudgmentPart::NOTE) == SKJ_NONE);
	}

	SECTION("Late window misses work as intended") {
		TestRuntimeNote t(
			otoworm::NoteData { 0, 0 }
		);
		double latemiss = s.sk->get_late_miss_cutoff_ms();

		int misses = s.sk->get_judgment_count(SKJ_MISS);
		REQUIRE(s.mech.on_update(latemiss + epsilon, t.get(), 0));
		REQUIRE(s.sk->get_judgment_count(SKJ_MISS) == misses + 1);
	}

	SECTION("No runtime errors across a big range of time") {
	    for (int i = -TIME_RANGE; i <= TIME_RANGE; i++) {
	        double t = (double)i / 1000.0;
            REQUIRE_NOTHROW(s.sk->hit_note(t, 0, NoteJudgmentPart::NOTE));
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
		REQUIRE_FALSE(s.mech.on_update(0, t.get(), 0));

		TestRuntimeNote t2(otoworm::NoteData{
			100, 100 + tailTime
			});

		// that is outside the judgment area
		REQUIRE_FALSE(s.mech.on_update(0, t2.get(), 0));
	}
	
	SECTION("Tails are not missed when the head is still active") {
		double tailTime = 0.001;
		double missCutoff = s.sk->get_late_miss_cutoff_ms() / 1000.0;

		TestRuntimeNote t(otoworm::NoteData{
			0, tailTime
		});

		// Simple tail time < time
		REQUIRE_FALSE(s.mech.on_update(0.06, t.get(), 0));

		// Tail time < time, head can still be hit, should not miss!
		REQUIRE_FALSE(s.mech.on_update(tailTime + missCutoff - 0.002, t.get(), 0));
	}

    SECTION("No runtime errors across a big range of time") {
        for (int i = -TIME_RANGE; i <= TIME_RANGE; i++) {
            double t = (double)i / 1000.0;
            REQUIRE_NOTHROW(s.sk->hit_note(t, 0, NoteJudgmentPart::NOTE));
        }
    }
}
