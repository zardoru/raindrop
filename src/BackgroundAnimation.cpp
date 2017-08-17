#include "pch.h"

#include "GameGlobal.h"
#include "Sprite.h"
#include "Song.h"
#include "BackgroundAnimation.h"
#include "Song7K.h"
#include "SongDC.h"
#include "ImageLoader.h"
#include "ImageList.h"
#include "Logging.h"
#include "osuBackgroundAnimation.h"
#include "VideoPlayback.h"

std::filesystem::path GetSongBackground(Game::Song &Song)
{
    auto SngDir = Song.SongDirectory;

    if (std::filesystem::exists(SngDir / Song.BackgroundFilename))
        return SngDir / Song.BackgroundFilename;

    for (auto i : std::filesystem::directory_iterator(SngDir))
    {
        std::string ext = i.path().extension().string();
		auto s = i.path().string();
        if (s.find("bg") != std::string::npos && (ext == ".jpg" || ext == ".png"))
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
	for (auto ext : videoextensions) {
		if (pathext == ext)
			return true;
	}

	return false;
}

class BMSBackground : public BackgroundAnimation
{
    std::shared_ptr<Sprite> Layer0;
    std::shared_ptr<Sprite> LayerMiss;
    std::shared_ptr<Sprite> Layer1;
    std::shared_ptr<Sprite> Layer2;
    std::vector<AutoplayBMP> EventsLayer0;
    std::vector<AutoplayBMP> EventsLayerMiss;
    std::vector<AutoplayBMP> EventsLayer1;
    std::vector<AutoplayBMP> EventsLayer2;

	std::map<int, VideoPlayback*> Videos;
	

    ImageList List;
    Game::VSRG::Song* Song;
    Game::VSRG::Difficulty* Difficulty;
    bool Validated;
    bool BlackToTransparent;
	int MaxWidth, MaxHeight;
	bool IsBMSON;
public:
    BMSBackground(Interruptible* parent, Game::VSRG::Difficulty* Difficulty, Game::VSRG::Song* Song) : BackgroundAnimation(parent), List(this)
    {
        this->Difficulty = Difficulty;
        this->Song = Song;
        Validated = false;
        MissTime = 0;

		MaxWidth = MaxHeight = 256;

        bool BtoT = false;
        if (Difficulty->Data->TimingInfo->GetType() == Game::VSRG::TI_BMS)
        {
			IsBMSON = std::dynamic_pointer_cast<Game::VSRG::BMSChartInfo>(Difficulty->Data->TimingInfo)->IsBMSON;
            if (!std::dynamic_pointer_cast<Game::VSRG::BMSChartInfo>(Difficulty->Data->TimingInfo)->IsBMSON)
                BtoT = true;
        }
        BlackToTransparent = BtoT;
    }

	~BMSBackground()
	{
		for (auto vid : Videos) {
			delete vid.second;
		}
	}

    void Load() override
    {
        EventsLayer0 = Difficulty->Data->BMPEvents->BMPEventsLayerBase;
        EventsLayerMiss = Difficulty->Data->BMPEvents->BMPEventsLayerMiss;
        EventsLayer1 = Difficulty->Data->BMPEvents->BMPEventsLayer;
        EventsLayer2 = Difficulty->Data->BMPEvents->BMPEventsLayer2;

		for (auto v : Difficulty->Data->BMPEvents->BMPList) {
			std::filesystem::path vs = v.second;
			std::filesystem::path path = Song->SongDirectory / vs;
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

        List.AddToList(Song->BackgroundFilename, Song->SongDirectory);
        List.LoadAll();
    }

    void Validate() override
    {
        if (Validated) return;

        Layer0 = std::make_shared<Sprite>();
        LayerMiss = std::make_shared<Sprite>();
        Layer1 = std::make_shared<Sprite>();
        Layer2 = std::make_shared<Sprite>();

        Layer0->ChainTransformation(&Transform);
        LayerMiss->ChainTransformation(&Transform);
        Layer1->ChainTransformation(&Transform);
        Layer2->ChainTransformation(&Transform);

        Layer0->SetZ(0);
        Layer1->SetZ(0);
        LayerMiss->SetZ(0);
        Layer2->SetZ(0);

        Layer1->BlackToTransparent = Layer2->BlackToTransparent = BlackToTransparent;

        LayerMiss->SetImage(List.GetFromIndex(0), true);
        Layer0->SetImage(List.GetFromIndex(1), true);


		auto ratio = Layer0->GetWidth() / Layer0->GetHeight();
		Layer0->SetWidth(1);
		Layer0->SetHeight(1);

        sort(EventsLayer0.begin(), EventsLayer0.end());
        sort(EventsLayerMiss.begin(), EventsLayerMiss.end());
        sort(EventsLayer1.begin(), EventsLayer1.end());
        sort(EventsLayer2.begin(), EventsLayer2.end());

        // Add BMP 0 as default value for layer 0. I was opting for a
        // if() at SetLayerImage time, but we're microoptimizing for branch mishits.
        if (EventsLayerMiss.size() == 0 || (EventsLayerMiss.size() > 0 && EventsLayerMiss[0].Time > 0))
        {
            AutoplayBMP bmp;
            bmp.Time = 0;
            bmp.BMP = 0;
            EventsLayerMiss.push_back(bmp);
        }



        Transform.SetWidth(256 * ratio);
        Transform.SetHeight(256);

        Validated = true;
    }

    void SetLayerImage(Sprite *sprite, std::vector<AutoplayBMP> &events_layer, double time)
    {
        auto bmp = std::lower_bound(events_layer.begin(), events_layer.end(), time, TimeSegmentCompare<AutoplayBMP>);
        if (bmp != events_layer.begin())
        {
            bmp = bmp - 1;

			auto tex = List.GetFromIndex(bmp->BMP);
			auto vid = dynamic_cast<VideoPlayback*>(tex);
			if (vid) {
				vid->UpdateClock(time - bmp->Time);
			}

            sprite->SetImage(tex, false);
        }
        else
        {
            //if (bmp != events_layer.end())
            //    sprite->SetImage(List.GetFromIndex(bmp->BMP), false);
            //else
                sprite->SetImage(nullptr, false);
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
        Log::Printf("Using static background: %s\n", Filename.string().c_str());
        List.AddToListIndex(Filename, 0);
    }

    void SetAnimationTime(double Time) override {}

    void Validate() override
    {
        if (!Background)
        {
            auto pt = List.GetFromIndex(0);
            Background = std::make_shared<Sprite>();
            Background->SetImage(pt, false);
            Background->ChainTransformation(&Transform);
            Transform.SetWidth(pt ? pt->w : 0);
            Transform.SetHeight(pt ? pt->h : 0);
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

std::unique_ptr<BackgroundAnimation> CreateBGAforVSRG(Game::VSRG::Song &input, uint8_t DifficultyIndex, Interruptible *context)
{
    Game::VSRG::Difficulty* Diff = input.GetDifficulty(DifficultyIndex);
    if (Diff)
    {
        if (Diff->Data && Diff->Data->BMPEvents)
            return std::make_unique<BMSBackground>(context, Diff, &input);
		if (Diff->Data && Diff->Data->TimingInfo && Diff->Data->TimingInfo->GetType() == Game::VSRG::TI_OSUMANIA)
			return std::make_unique<osuBackgroundAnimation>(context, Diff->Data->osbSprites.get(), &input);

        return std::make_unique<StaticBackground>(context, GetSongBackground(input));
    }

    return nullptr;
}

std::unique_ptr<BackgroundAnimation> CreateBGAforDotcur(Game::dotcur::Song &input, uint8_t DifficultyIndex)
{
    return std::make_unique<StaticBackground>(nullptr, GetSongBackground(input));
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

Transformation& BackgroundAnimation::GetTransformation()
{
    return Transform;
}

std::unique_ptr<BackgroundAnimation> BackgroundAnimation::CreateBGAFromSong(uint8_t DifficultyIndex, Game::Song& Input, Interruptible* context, bool LoadNow)
{
    std::unique_ptr<BackgroundAnimation> ret = nullptr;

    switch (Input.Mode)
    {
    case MODE_VSRG:
        ret = CreateBGAforVSRG(static_cast<Game::VSRG::Song&> (Input), DifficultyIndex, context);
        break;
    case MODE_DOTCUR:
        ret = CreateBGAforDotcur(static_cast<Game::dotcur::Song&> (Input), DifficultyIndex);
        break;
    default:
        break;
    }

    if (LoadNow)
    {
        ret->Load();
        ret->Validate();
    }
    return ret;
}