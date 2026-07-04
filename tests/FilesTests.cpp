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
#include <client/songdb/SongLoader.h>
#include <ProcessedChart.h>

TEST_CASE("osu storyboard compliance")
{
	Interruptible stub;
	auto chart_group = LoadChartGroupFromFilename("tests/files/esb.osu");

	REQUIRE(chart_group != nullptr);
	REQUIRE(!chart_group->charts.empty());

	auto bga = BackgroundAnimation::CreateBGAFromChartGroup(0, chart_group, &stub, true);

	bga->SetAnimationTime(65.0f);
}

TEST_CASE("Speed support")
{
	auto chart_group = LoadChartGroupFromFilename("tests/files/jnight.ssc");
	auto pcd = otoworm::ProcessedChart::from(chart_group->get_chart(0));
	auto tbeat = pcd.get_time_for_beat(93. + 4.);
	REQUIRE(pcd.get_speed_multiplier_at(tbeat) == 0.250);
}
