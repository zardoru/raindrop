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
	So much of the testing was already done by debugging the main .exe
	further new code will comply for use in test-driven development (2016-05-23)
	but for now I'll use this to make debugging easier. -az
*/
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

TEST_CASE("Noteskin can be used with no context", "[noteskin]")
{
	Game::VSRG::Noteskin n(nullptr);

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

TEST_CASE("Raindrop Mechanics (No forced release)", "[raindropmechs]") {
	using namespace Game::VSRG;
	auto sk = std::make_shared<ScoreKeeper>();
	RaindropMechanics mech(false);
	mech.Setup(nullptr, sk);
	auto missCutoff = sk->getMissCutoffMS() / 1000;

	SECTION("Tails are not missed earlier than they should") {
		double tailTime = 0.001;
		TrackNote t(NoteData{
			0, tailTime
			});

		// that is inside the judgement area
		REQUIRE_FALSE(mech.OnUpdate(0, &t, 0));

		TrackNote t2(NoteData{
			100, 100 + tailTime
			});

		// that is outside the judgment area
		REQUIRE_FALSE(mech.OnUpdate(0, &t2, 0));
	}
	
	SECTION("Tails are not missed when the head is still active") {
		double tailTime = 0.001;
		TrackNote t(NoteData{
			0, tailTime
			});

		// Simple tail time < time
		REQUIRE_FALSE(mech.OnUpdate(0.06, &t, 0));

		// Tail time < time, head can still be hit, should not miss!
		REQUIRE_FALSE(mech.OnUpdate(tailTime + missCutoff - 0.002, &t, 0));
	}
}