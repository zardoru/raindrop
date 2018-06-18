#include "pch.h"

#include "../src/GameGlobal.h"
#include "../src/Song.h"
#include "../src/Song7K.h"
#include "../src/SongLoader.h"
#include "../src/BackgroundAnimation.h"

#include "../src/LuaManager.h"
#include "../src/Noteskin.h"

#include "../src/PlayerChartData.h"

#include "../src/Logging.h"
#include "../src/ext/catch.hpp"
#include "../src/VSRGMechanics.h"
#include "../src/ScoreKeeper7K.h"

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

using namespace Game::VSRG;

TEST_CASE("Noteskin can be used with no context", "[noteskin]")
{
	Noteskin n(nullptr);

	SECTION("Noteskin doesn't crash with no context") {
		
		REQUIRE_NOTHROW(n.SetupNoteskin(false, 4));
		REQUIRE_NOTHROW(n.Validate());
		REQUIRE_NOTHROW(n.DrawHoldBody(0, 0, 0, 0));
	}
}

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

		REQUIRE_THROWS(s.sk->hitNote(earlymiss - 1));
		REQUIRE_THROWS(s.sk->hitNote(latemiss + 1));
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
		double missCutoff = s.sk->getLateMissCutoffMS();

		TrackNote t(NoteData{
			0, tailTime
		});

		// Simple tail time < time
		REQUIRE_FALSE(s.mech.OnUpdate(0.06, &t, 0));

		// Tail time < time, head can still be hit, should not miss!
		REQUIRE_FALSE(s.mech.OnUpdate(tailTime + missCutoff - 0.002, &t, 0));
	}
}

struct OMSetup
{
	std::shared_ptr<ScoreKeeper> sk;
	RaindropMechanics mech;

	OMSetup() : mech(true) {
		sk = std::make_shared<ScoreKeeper>();
		sk->setODWindows(0);
		mech.Setup(nullptr, sk);
		sk->setMaxNotes(100);
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

}