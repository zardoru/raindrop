#include <cstdint>
#include <filesystem>
#include <map>
#include <thread>
#include <rmath.h>
#include <sstream>

#include "../structure/Configuration.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include <ChartGroup.h>

#include "BackgroundAnimation.h"

#include <algorithm>

#include "Texture.h"
#include "VideoPlayback.h"

#include "ImageLoader.h"
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

bool IsVideoPath(std::filesystem::path path)
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

    void Load() override
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
			if (IsVideoPath(path))
			{
				auto vid = new VideoPlayback();
				if (vid->Open(path)) {
					vid->StartDecodeThread();
					List.AddToListIndex(vid, v.first);
					Videos[v.first] = vid;
					MaxWidth = std::max(MaxWidth, vid->w);
					MaxHeight = std::max(MaxHeight, vid->h);
				}
				else
					delete vid;
			}
			else
				List.AddToListIndex(path, v.first);

		}

        List.AddToList(BackgroundFilename, SongDirectory);
        List.LoadAll();
    }

    void Validate() override
    {
        if (Validated) return;

        Layer0 = std::make_shared<Sprite>();
        LayerMiss = std::make_shared<Sprite>();
        Layer1 = std::make_shared<Sprite>();
        Layer2 = std::make_shared<Sprite>();

        Layer0->ChainTransformation(this);
        LayerMiss->ChainTransformation(this);
        Layer1->ChainTransformation(this);
        Layer2->ChainTransformation(this);

        Layer0->SetZ(0);
        Layer1->SetZ(0);
        LayerMiss->SetZ(0);
        Layer2->SetZ(0);

        Layer1->BlackToTransparent = Layer2->BlackToTransparent = BlackToTransparent;

        LayerMiss->set_image(List.GetFromIndex(0), true);
        Layer0->set_image(List.GetFromIndex(1), true);


		auto ratio = Layer0->GetWidth() / Layer0->GetHeight();
		Layer0->SetWidth(1);
		Layer0->SetHeight(1);

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

        SetWidth(256 * ratio);
        SetHeight(256);

        Validated = true;
    }

    void SetLayerImage(Sprite *sprite, std::vector<BGAEvent> &events_layer, double time)
    {
        auto bmp = std::lower_bound(events_layer.begin(), events_layer.end(), time, BGAEventTimeBefore);
        if (bmp != events_layer.begin())
        {
            bmp = bmp - 1;

			auto tex = List.GetFromIndex(bmp->BMP);
            if (const auto vid = dynamic_cast<VideoPlayback*>(tex)) {
				vid->UpdateClock(time - bmp->Time);
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

    void SetAnimationTime(double Time) override
    {
        if (!Validated) return;

        SetLayerImage(Layer0.get(), EventsLayer0, Time);
        SetLayerImage(LayerMiss.get(), EventsLayerMiss, Time);
        SetLayerImage(Layer1.get(), EventsLayer1, Time);
        SetLayerImage(Layer2.get(), EventsLayer2, Time);
    }

    float MissTime;

    void Render() override
    {
        Layer0->Render();
        Layer1->Render();
        Layer2->Render();

        if (MissTime > 0)
            LayerMiss->Render();
    }

    void OnMiss() override
    {
        MissTime = Configuration::GetSkinConfigf("OnMissBGATime");
    }

    void Update(float Delta) override
    {
        MissTime -= Delta;
    }
};

class StaticBackground : public BackgroundAnimation
{
    std::shared_ptr<Sprite> Background;
    ImageList List;
public:
    StaticBackground(Interruptible* parent, std::filesystem::path Filename)
        : BackgroundAnimation(parent), List(this)
    {
        Log::Printf("Using static background: %ls\n", Filename.wstring().c_str());
        List.AddToListIndex(Filename, 0);
    }

    void SetAnimationTime(double Time) override {}

    void Validate() override
    {
        if (!Background)
        {
            auto pt = List.GetFromIndex(0);
            Background = std::make_shared<Sprite>();
            Background->set_image(pt, false);
            Background->ChainTransformation(this);
            SetWidth(pt ? pt->w : 0);
            SetHeight(pt ? pt->h : 0);
        }
    }

    void Load() override
    {
        List.LoadAll();
    }

    void Render() override
    {
        if (Background != nullptr)
            Background->Render();
    }
};

std::unique_ptr<BackgroundAnimation> CreateBGAforVSRG(
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

                return std::make_unique<osuBackgroundAnimation>(context, ReadOSBEvents(s), input->path);
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

void BackgroundAnimation::SetAnimationTime(double Time)
{
}

void BackgroundAnimation::Load()
{
}

void BackgroundAnimation::Validate()
{
}

void BackgroundAnimation::Update(float Delta)
{
}

void BackgroundAnimation::OnHit()
{
}

void BackgroundAnimation::OnMiss()
{
}

void BackgroundAnimation::Render()
{
}

std::unique_ptr<BackgroundAnimation> BackgroundAnimation::CreateBGAFromChartGroup(
        uint8_t chart_index,
        const std::shared_ptr<otoworm::ChartGroup>& chart_group,
        Interruptible* context,
        bool LoadNow)
{
    auto ret = CreateBGAforVSRG(chart_group, chart_index, context);
    
    if (ret && LoadNow)
    {
        ret->Load();
        ret->Validate();
    }

    return ret;
}
