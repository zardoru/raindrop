#include "pch.h"
#include "../src/GameGlobal.h"
#include "../src/Song.h"
#include "../src/SongDC.h"
#include "../src/Song7K.h"
#include "../src/SongLoader.h"
#include "../src/BackgroundAnimation.h"

#include "../src/LuaManager.h"
#include "../src/Noteskin.h"

#include "../src/PlayerChartData.h"

#include "../src/Logging.h"
#include "../src/ext/catch.hpp"
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

TEST_CASE("Lua Manager state")
{
	LuaManager l;
	REQUIRE(!l.CallFunction("NonExistingFunction"));
}

TEST_CASE("Noteskin state")
{
	Game::VSRG::Noteskin n(nullptr);
	n.SetupNoteskin(false, 4);
	n.Validate();
	n.DrawHoldBody(0,0,0,0);
}
*/

TEST_CASE("Speed support")
{
	auto sng = LoadSong7KFromFilename("tests/files/jnight.ssc");
	auto pcd = Game::VSRG::PlayerChartData::FromDifficulty(sng->GetDifficulty(0));
	auto tbeat = pcd.GetTimeAtBeat(93. + 4.);
	REQUIRE(pcd.GetSpeedMultiplierAt(tbeat) == 0.250);
}