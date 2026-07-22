/* tests that depend on outside files */

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <cstdint>
#include <future>
#include <sstream>
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
#include <note_loader.h>

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

TEST_CASE("BMS loader consumes evaluated event commands", "[bms]")
{
    const std::string text =
        "#TITLE Track [Hyper]\r\n"
        "#ARTIST Composer / obj: Chart author\r\n"
        "#BPM 120\r\n"
        "#WAV01 one.wav\r\n"
        "#WAV02 two.wav\r\n"
        "#00101:02000100\r\n"
        "#00111:0100\r\n";

    const auto tree = NoteLoaderBMS::ParseTreeFromString(text);
    REQUIRE(tree);

    const auto commands = tree->evaluate();
    const auto event = std::find_if(commands.begin(), commands.end(), [](const auto& command) {
        return command.type == otoworm::bms::command_type::events &&
            command.event_channel.kind == otoworm::bms::channel::bgm;
    });
    REQUIRE(event != commands.end());
    REQUIRE(event->measure == 1);
    REQUIRE(event->events == std::vector<uint16_t>{2, 0, 1, 0});

    std::istringstream stream(text);
    otoworm::ChartGroup song;
    NoteLoaderBMS::load_chart_from_stream(stream, &song);

    REQUIRE(song.title == "Track");
    REQUIRE(song.artist == "Composer");
    REQUIRE(song.charts.size() == 1);

    const auto& chart = song.charts.front();
    REQUIRE(chart->meta->name == "Hyper");
    REQUIRE(chart->meta->author == "Chart author");
    REQUIRE(chart->transient->bgm_events.size() == 2);
    REQUIRE(chart->transient->bgm_events[0].sound == 2);
    REQUIRE(chart->transient->bgm_events[1].sound == 1);
    REQUIRE(chart->transient->bgm_events[0].time < chart->transient->bgm_events[1].time);
}
