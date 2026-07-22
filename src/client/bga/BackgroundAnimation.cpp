#include <cstdint>
#include <filesystem>
#include <map>
#include <thread>
#include <rmath.h>
#include <sstream>

#include "../structure/Configuration.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Shader.h"
#include "Sprite.h"
#include <ChartGroup.h>

#include "BackgroundAnimation.h"

#include <algorithm>

#include "Texture2D.h"
#include "VideoPlayback.h"

#include "TextureCollection.h"
#include "ImageList.h"
#include "Logging.h"

#include <game/Timing.h>
#include "osuBackgroundAnimation.h"

namespace
{
    struct BGAEvent
    {
        double Time = 0;
        int BMP = 0;

        bool operator<(const BGAEvent& other) const
        {
            return Time < other.Time;
        }
    };

    bool BGAEventTimeBefore(const BGAEvent& event, const double time)
    {
        return event.Time < time;
    }

    BGAEvent ConvertAutoplayBMP(const otoworm::AutoplayBMP& event)
    {
        return {event.time, event.bmp};
    }

    std::vector<BGAEvent> ConvertAutoplayBMPEvents(const std::vector<otoworm::AutoplayBMP>& events)
    {
        std::vector<BGAEvent> out;
        out.reserve(events.size());
        for (const auto& event : events) {
            out.push_back(ConvertAutoplayBMP(event));
        }
        return out;
    }

    otoworm::ChartTransient* GetOtoTransient(const std::shared_ptr<otoworm::Chart>& chart)
    {
        if (!chart) {
            return nullptr;
        }
        return chart->transient.get();
    }

    const otoworm::BMPEventsDetail* GetOtoBMPEvents(const std::shared_ptr<otoworm::Chart>& chart)
    {
        auto* transient = GetOtoTransient(chart);
        if (!transient || !transient->bmp_events) {
            return nullptr;
        }
        return &*transient->bmp_events;
    }

    bool IsOtoBmson(const std::shared_ptr<otoworm::Chart>& chart)
    {
        auto* transient = GetOtoTransient(chart);
        if (!transient || !transient->specialized_info ||
            transient->specialized_info->get_class() != otoworm::CC_BMS) {
            return false;
        }

        return std::static_pointer_cast<otoworm::BMSChartInfo>(transient->specialized_info)->is_bmson;
    }

    std::string GetOtoOsuSprites(const std::shared_ptr<otoworm::Chart>& chart)
    {
        auto* transient = GetOtoTransient(chart);
        auto* osu_transient = dynamic_cast<otoworm::OsumaniaChartTransient*>(transient);
        return osu_transient ? osu_transient->osb_sprites : "";
    }
}


std::filesystem::path GetSongBackground(const otoworm::ChartGroup& chart_group)
{
    auto SngDir = chart_group.path;

    if (std::filesystem::exists(SngDir / chart_group.background_filename))
        return SngDir / chart_group.background_filename;

    for (auto i : std::filesystem::directory_iterator(SngDir))
    {
        std::string ext = i.path().extension().string();
		const auto& s = i.path();
        if (s.wstring().find(L"bg") != std::string::npos && (ext == ".jpg" || ext == ".png"))
            return i;
    }

    return Configuration::GetSkinConfigs("DefaultBackground");
}

std::string videoextensions[] = {
	".mkv",
	".mp4",
	".avi",
	".wmv",
	".m4v",
	".mpg",
	".mpeg",
	".mpv",
	".flv",
	".webm"
};

bool is_video_path(std::filesystem::path path)
{
	auto pathext = path.extension().string();
	for (const auto& ext : videoextensions) {
		if (pathext == ext)
			return true;
	}

	return false;
}

class BMSBackground final : public BackgroundAnimation
{
    std::shared_ptr<Sprite> Layer0;
    std::shared_ptr<Sprite> LayerMiss;
    std::shared_ptr<Sprite> Layer1;
    std::shared_ptr<Sprite> Layer2;
    std::vector<BGAEvent> EventsLayer0;
    std::vector<BGAEvent> EventsLayerMiss;
    std::vector<BGAEvent> EventsLayer1;
    std::vector<BGAEvent> EventsLayer2;

	std::map<int, VideoPlayback*> Videos;
	

    ImageList List;
    std::filesystem::path SongDirectory;
    std::filesystem::path BackgroundFilename;
    std::shared_ptr<otoworm::Chart> Chart;
    bool Validated;
    bool BlackToTransparent;
	int MaxWidth, MaxHeight;
	bool IsBMSON;
public:
    BMSBackground(
            Interruptible* parent,
            std::shared_ptr<otoworm::Chart> chart,
            std::filesystem::path song_directory,
            std::filesystem::path background_filename)
            : BackgroundAnimation(parent), List(this)
    {
        Chart = std::move(chart);
        SongDirectory = std::move(song_directory);
        BackgroundFilename = std::move(background_filename);
        Validated = false;
        MissTime = 0;

		MaxWidth = MaxHeight = 256;

        bool BtoT = false;
        IsBMSON = IsOtoBmson(Chart);
        if (!IsBMSON)
            BtoT = true;
        BlackToTransparent = BtoT;
    }

	~BMSBackground() final
	{
		for (auto vid : Videos) {
			delete vid.second;
		}
	}

    void load() override
    {
        const auto* bmp_events = GetOtoBMPEvents(Chart);
        if (!bmp_events) {
            return;
        }

        EventsLayer0 = ConvertAutoplayBMPEvents(bmp_events->layer_base);
        EventsLayerMiss = ConvertAutoplayBMPEvents(bmp_events->layer_miss);
        EventsLayer1 = ConvertAutoplayBMPEvents(bmp_events->layer_upper);
        EventsLayer2 = ConvertAutoplayBMPEvents(bmp_events->layer_upper2);

		for (const auto& v : bmp_events->bmp_list) {
			auto vs = v.second;
			auto path = SongDirectory / vs;
			if (is_video_path(path))
			{
				auto vid = new VideoPlayback();
				if (vid->open(path)) {
					vid->start_decode_thread();
					List.add_to_list_index(vid, v.first);
					Videos[v.first] = vid;
					MaxWidth = std::max(MaxWidth, vid->w);
					MaxHeight = std::max(MaxHeight, vid->h);
				}
				else
					delete vid;
			}
			else
				List.add_to_list_index(path, v.first);

		}

        List.add_to_list(BackgroundFilename, SongDirectory);
        List.load_all();
    }

    void finalize_loading() override
    {
        if (Validated) return;

        Layer0 = std::make_shared<Sprite>();
        LayerMiss = std::make_shared<Sprite>();
        Layer1 = std::make_shared<Sprite>();
        Layer2 = std::make_shared<Sprite>();

        Layer0->chain_transformation(this);
        LayerMiss->chain_transformation(this);
        Layer1->chain_transformation(this);
        Layer2->chain_transformation(this);

        Layer0->set_z(0);
        Layer1->set_z(0);
        LayerMiss->set_z(0);
        Layer2->set_z(0);

        LayerMiss->set_image(List.get_from_index(0), true);
        Layer0->set_image(List.get_from_index(1), true);


		const auto ratio = Layer0->get_width() / Layer0->get_height();
		Layer0->set_width(1);
		Layer0->set_height(1);

        sort(EventsLayer0.begin(), EventsLayer0.end());
        sort(EventsLayerMiss.begin(), EventsLayerMiss.end());
        sort(EventsLayer1.begin(), EventsLayer1.end());
        sort(EventsLayer2.begin(), EventsLayer2.end());

        // Add BMP 0 as default value for layer 0.
        if (EventsLayerMiss.empty() || (!EventsLayerMiss.empty() && EventsLayerMiss[0].Time > 0))
        {
            BGAEvent bmp;
            bmp.Time = 0;
            bmp.BMP = 0;
            EventsLayerMiss.push_back(bmp);
        }

        set_width(256 * ratio);
        set_height(256);

        if (BlackToTransparent)
            black_to_transparent_shader();

        Validated = true;
    }

    void set_layer_image(Sprite *sprite, std::vector<BGAEvent> &events_layer, double time)
    {
        auto bmp = std::lower_bound(events_layer.begin(), events_layer.end(), time, BGAEventTimeBefore);
        if (bmp != events_layer.begin())
        {
            bmp = bmp - 1;

			auto tex = List.get_from_index(bmp->BMP);
            if (const auto vid = dynamic_cast<VideoPlayback*>(tex)) {
				vid->update_clock(time - bmp->Time);
			}

            sprite->set_image(tex, false);
        }
        else
        {
            //if (bmp != events_layer.end())
            //    sprite->SetImage(List.GetFromIndex(bmp->BMP), false);
            //else
                sprite->set_image(nullptr, false);
        }
    }

    void set_animation_time(double Time) override
    {
        if (!Validated) return;

        set_layer_image(Layer0.get(), EventsLayer0, Time);
        set_layer_image(LayerMiss.get(), EventsLayerMiss, Time);
        set_layer_image(Layer1.get(), EventsLayer1, Time);
        set_layer_image(Layer2.get(), EventsLayer2, Time);
    }

    float MissTime;

    void emit_draw_calls(DrawCallSink &sink) override
    {
        Layer0->emit_draw_calls(sink);
        auto *shader = BlackToTransparent ? black_to_transparent_shader() : nullptr;
        Layer1->emit_draw_calls(sink, shader);
        Layer2->emit_draw_calls(sink, shader);

        if (MissTime > 0)
            LayerMiss->emit_draw_calls(sink);
    }

    void on_miss() override
    {
        MissTime = Configuration::GetSkinConfigf("OnMissBGATime");
    }

    void update(float Delta) override
    {
        MissTime -= Delta;
    }
};

class StaticBackground : public BackgroundAnimation
{
    std::shared_ptr<Sprite> background_;
    ImageList list_;
public:
    StaticBackground(Interruptible* parent, const std::filesystem::path &filename)
        : BackgroundAnimation(parent), list_(this)
    {
        Log::Printf("Using static background: %ls\n", filename.wstring().c_str());
        list_.add_to_list_index(filename, 0);
    }

    void set_animation_time(double Time) override {}

    void finalize_loading() override
    {
        if (!background_)
        {
            auto pt = list_.get_from_index(0);
            background_ = std::make_shared<Sprite>();
            background_->set_image(pt, false);
            background_->chain_transformation(this);
            set_width(pt ? pt->w : 0);
            set_height(pt ? pt->h : 0);
        }
    }

    void load() override
    {
        list_.load_all();
    }

    void emit_draw_calls(DrawCallSink &sink) override
    {
        if (background_ != nullptr)
            background_->emit_draw_calls(sink);
    }
};

std::unique_ptr<BackgroundAnimation> make_bga(
        const std::shared_ptr<otoworm::ChartGroup>& input,
        uint8_t chart_index,
        Interruptible *context)
{
    if (input && chart_index < input->charts.size())
    {
        auto chart = input->charts[chart_index];
        if (GetOtoBMPEvents(chart))
            return std::make_unique<BMSBackground>(
                    context,
                    chart,
                    input->path,
                    input->background_filename);
        auto osb_sprites = GetOtoOsuSprites(chart);
		if (!osb_sprites.empty()) {
		    try {
		        std::stringstream s(osb_sprites);

                return std::make_unique<osuBackgroundAnimation>(context, read_osb_events(s), input->path);
            } catch (std::exception &e) {
                Log::LogPrintf("Failure to parse OSB events of .osu file. Reason: %s\n", e.what());
            }
        }

        return std::make_unique<StaticBackground>(context, GetSongBackground(*input));
    }

    return nullptr;
}

BackgroundAnimation::BackgroundAnimation(Interruptible* parent) : Interruptible(parent)
{
}

BackgroundAnimation::~BackgroundAnimation() = default;

renderer::Shader::BGA *BackgroundAnimation::black_to_transparent_shader()
{
    if (!black_to_transparent_shader_) {
        // This links against Shader::Default's vertex shader, which is compiled during window setup.
        black_to_transparent_shader_ = std::make_unique<renderer::Shader::BGA>();
    }

    return black_to_transparent_shader_.get();
}

void BackgroundAnimation::set_animation_time(double Time)
{
}

void BackgroundAnimation::load()
{
}

void BackgroundAnimation::finalize_loading()
{
}

void BackgroundAnimation::update(float Delta)
{
}

void BackgroundAnimation::on_hit()
{
}

void BackgroundAnimation::on_miss()
{
}

void BackgroundAnimation::emit_draw_calls(DrawCallSink &sink)
{
}

std::unique_ptr<BackgroundAnimation> BackgroundAnimation::create_bga_from_chart_group(
        const uint8_t chart_index,
        const std::shared_ptr<otoworm::ChartGroup>& chart_group,
        Interruptible* context,
        const bool load_now)
{
    auto ret = make_bga(chart_group, chart_index, context);
    
    if (ret && load_now)
    {
        ret->load();
        ret->finalize_loading();
    }

    return ret;
}
