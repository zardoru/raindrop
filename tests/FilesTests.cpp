/* tests that depend on outside files */

#include <catch.hpp>
#include <filesystem>
#include <cstdint>
#include <rmath.h>
#include <map>

#include <client/backend/Transformation.h>
#include <client/backend/Rendering.h>
#include <client/backend/Sprite.h>
#include <client/bga/BackgroundAnimation.h>
#include <game/Song.h>
#include <client/songdb/SongLoader.h>
#include <game/PlayerChartState.h>

TEST_CASE("osu storyboard compliance")
{
	Interruptible stub;
	auto song = LoadSong7KFromFilename("tests/files/esb.osu");

	REQUIRE(song != nullptr);
	REQUIRE(!song->Difficulties.empty());

	auto bga = BackgroundAnimation::CreateBGAFromSong(0, *song, &stub, true);

	bga->SetAnimationTime(65.0f);
}

TEST_CASE("Speed support")
{
	auto sng = LoadSong7KFromFilename("tests/files/jnight.ssc");
	auto pcd = rd::PlayerChartState::FromDifficulty(sng->GetDifficulty(0));
	auto tbeat = pcd.GetTimeAtBeat(93. + 4.);
	REQUIRE(pcd.GetSpeedMultiplierAt(tbeat) == 0.250);
}
