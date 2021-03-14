#include <filesystem>

#include <catch.hpp>
#include <LuaManager.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper7K.h>

constexpr auto TIME_RANGE = 10000;

/*
TEST_CASE("osu storyboard compliance")
{
	Interruptible stub;
	auto song = LoadSong7KFromFilename("tests/files/esb.osu");
	
	REQUIRE(song != nullptr);
	REQUIRE(song->Difficulties.size() != 0);

	Log::Printf("Loading BGA...\n");
	auto bga = BackgroundAnimation::CreateBGAFromSong(0, *song, &stub, true);

	bga->SetAnimationTime(65.0f);
}
*/

TEST_CASE("Lua Manager state")
{
	LuaManager l;
	REQUIRE_FALSE(l.CallFunction("NonExistingFunction"));
}

using namespace rd;


/*
TEST_CASE("Speed support")
{
	auto sng = LoadSong7KFromFilename("tests/files/jnight.ssc");
	auto pcd = Game::VSRG::PlayerChartState::FromDifficulty(sng->GetDifficulty(0));
	auto tbeat = pcd.GetTimeAtBeat(93. + 4.);
	REQUIRE(pcd.GetSpeedMultiplierAt(tbeat) == 0.250);
}
*/

double epsilon = 0.001; // one ms

struct BMSSetup {
	std::shared_ptr<ScoreKeeper> sk;
	RaindropMechanics mech;

	BMSSetup() : mech(true) {
		sk = std::make_shared<ScoreKeeper>();
		sk->setJudgeRank(4); // easy
		mech.Setup(nullptr, sk);
		sk->setMaxNotes(100);

		mech.MissNotify = [&](double t, uint32_t, bool hold, bool nobreakcombo, bool earlymiss) {
			sk->missNote(nobreakcombo, earlymiss);
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
		sk->setMaxNotes(100);
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

		auto t1 = t.GetStartTime() - earlyMiss;
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
		REQUIRE(s.sk->hitNote(hitwindow) != SKJ_MISS);
		REQUIRE(s.sk->hitNote(hitwindow) != SKJ_NONE);
		REQUIRE(s.sk->hitNote(hitwindow - 1) != SKJ_MISS);
		REQUIRE(s.sk->hitNote(hitwindow - 1) != SKJ_NONE);
	}

	SECTION("NONE judgment outside of hit window") {
		double earlymiss = -s.sk->getEarlyMissCutoffMS();
		double latemiss = s.sk->getLateMissCutoffMS();

		REQUIRE(s.sk->hitNote(earlymiss - 1) == SKJ_NONE);
		REQUIRE(s.sk->hitNote(latemiss + 1) == SKJ_NONE);
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
            REQUIRE_NOTHROW(s.sk->hitNote(t));
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
            REQUIRE_NOTHROW(s.sk->hitNote(t));
        }
    }
}

struct OMSetup
{
	std::shared_ptr<ScoreKeeper> sk;
	RaindropMechanics mech;

	// always lane 0
	bool LaneDown;

	OMSetup() : mech(true) {
		sk = std::make_shared<ScoreKeeper>();
		sk->setODWindows(0);
		mech.Setup(nullptr, sk);
		sk->setMaxNotes(100);

		// SetLaneHoldingState is set if we're currently hitting a hold.
		// we don't really need this.
		/*mech.SetLaneHoldingState = [&](uint32_t, bool state) {
			LaneDown = state;
		};*/

		mech.IsLaneKeyDown = [&](uint32_t) {
			return LaneDown;
		};

		mech.MissNotify = [&](double t, uint32_t, bool hold, bool nobreakcombo, bool earlymiss) {
			sk->missNote(nobreakcombo, earlymiss);
		};
	}
};

TEST_CASE("osu!mania judgments", "[omjudge]") {
	OMSetup s;

	double JudgmentValues[] = { -16, -64, -97, -127, -151, -188 };

	double JudgmentValuesLate[] = { 15, 63, 96, 126, 150 };

	SECTION("Judgments from W1 to W5 are correct Early (OD0)") {
		auto j = SKJ_W1;
		for (auto judgems : JudgmentValues) {
			REQUIRE(s.sk->hitNote(judgems + 2) == j);

			j = (ScoreKeeperJudgment)((int)j + 1);
		}
	}

	SECTION("Judgments from W1 to W5 are correct at the limit Early (OD0)") {
		auto j = SKJ_W1;
		for (auto judgems : JudgmentValues) {
			REQUIRE(s.sk->hitNote(judgems) == j);

			j = (ScoreKeeperJudgment)((int)j + 1);
		}
	}

	SECTION("Judgments from W1 to W5 are correct at the limit Late (OD0)") {
		auto j = SKJ_W1;
		for (auto judgems : JudgmentValuesLate) {
			REQUIRE(s.sk->hitNote(judgems) == j);

			j = (ScoreKeeperJudgment)((int)j + 1);
		}
	}

	double earlyhitWindow = s.sk->getEarlyHitCutoffMS() / 1000.0;
	double lateCutoff = s.sk->getLateMissCutoffMS() / 1000.0;
	double lnCutoff = s.sk->getJudgmentWindow(SKJ_W3);
	TrackNote t(
		NoteData{ 0, 10 }
	);

	SECTION("Long note tail windows are hit inside the hit window") {
		TrackNote lt = t;

		lt.Hit(); // we need this flag to hit the tail
		REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime(), &lt, 0));

		lt.Reset(); lt.Hit();
		REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() - earlyhitWindow + epsilon, &lt, 0));

		lt.Reset(); lt.Hit();
		REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() + lateCutoff - epsilon, &lt, 0));
	}

	SECTION("Long note tails have proper lenience when hit") {
		TrackNote lt = t;

		// update
		lt.Hit();
		REQUIRE_FALSE(s.mech.OnUpdate(lt.GetEndTime() - lateCutoff + epsilon, &lt, 0));
		REQUIRE_FALSE(s.mech.OnUpdate(lt.GetEndTime() + lateCutoff - epsilon, &lt, 0));
		REQUIRE_FALSE(s.mech.OnUpdate(lt.GetEndTime() + epsilon, &lt, 0));

		lt.Reset(); lt.Hit();
		REQUIRE(s.mech.OnUpdate(lt.GetEndTime() + lateCutoff + epsilon, &lt, 0));


		// release on time
		lt.Reset(); lt.Hit();
		int misses = s.sk->getJudgmentCount(SKJ_MISS);
		REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() + lateCutoff - epsilon, &lt, 0));
		REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses);

		lt.Reset(); lt.Hit();
		REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() + epsilon, &lt, 0));
		REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses);

		lt.Reset(); lt.Hit();
		REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() - lateCutoff + epsilon, &lt, 0));
		REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses);

		// too early/late release
		lt.Reset(); lt.Hit();
		REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() - earlyhitWindow - epsilon, &lt, 0));
		REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses + 1);

		lt.Reset(); lt.Hit();
		REQUIRE(s.mech.OnReleaseLane(lt.GetEndTime() + lateCutoff + epsilon, &lt, 0));
		REQUIRE(s.sk->getJudgmentCount(SKJ_MISS) == misses + 2);
	}

	SECTION("Long note tails miss only after the tail end is done when not hit") {
		TrackNote lt = t;
		REQUIRE_FALSE(s.mech.OnUpdate(5, &lt, 0));

		lt.DisableHead(); // otherwise, it'll miss the head
		REQUIRE_FALSE(s.mech.OnUpdate(t.GetEndTime() - epsilon, &lt, 0));
		REQUIRE(s.mech.OnUpdate(t.GetEndTime() + epsilon, &lt, 0));
	}


    SECTION("No runtime errors across a big range of time") {
        for (int i = -TIME_RANGE; i <= TIME_RANGE; i++) {
            double t = (double)i / 1000.0;
            REQUIRE_NOTHROW(s.sk->hitNote(t));
        }
    }
}