/* tests that depend on outside files */

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <cstdint>
#include <future>
#include <vector>
#include <rmath.h>
#include <map>

#include <client/backend/Transformation.h>
#include <client/backend/Rendering.h>
#include <client/backend/Sprite.h>
#include <client/bga/BackgroundAnimation.h>
#include <client/songdb/SongLoader.h>
#include <ProcessedChart.h>
#include <sndio/Audiofile.h>
#include <sndio/AudioSourceOGG.h>

TEST_CASE("osu storyboard compliance")
{
	Interruptible stub;
	auto chart_group = LoadChartGroupFromFilename("tests/files/esb.osu");

	REQUIRE(chart_group != nullptr);
	REQUIRE(!chart_group->charts.empty());

	auto bga = BackgroundAnimation::create_bga_from_chart_group(0, chart_group, &stub, true);

	bga->set_animation_time(65.0f);
}

TEST_CASE("Speed support")
{
	auto chart_group = LoadChartGroupFromFilename("tests/files/jnight.ssc");
	auto pcd = otoworm::ProcessedChart::from(chart_group->get_chart(0));
	auto tbeat = pcd.get_time_for_beat(93. + 4.);
	REQUIRE(pcd.get_speed_multiplier_at(tbeat) == 0.250);
}

TEST_CASE("OGG audio source reports frames and returns samples", "[audio]")
{
	AudioSourceOGG source;

	REQUIRE(source.open("data/skins/default/loop1.ogg"));
	REQUIRE(source.get_rate() == 44100);
	REQUIRE(source.get_channels() == 2);
	REQUIRE(source.get_length() > 0);

	std::vector<short> buffer(4096);
	size_t total_samples_read = 0;

	while (source.has_data_left()) {
		const auto samples_read = source.read(buffer.data(), buffer.size());

		REQUIRE(samples_read <= buffer.size());
		total_samples_read += samples_read;

		if (!samples_read)
			break;
	}

	REQUIRE(total_samples_read == source.get_length() * source.get_channels());
	REQUIRE_FALSE(source.has_data_left());
}
