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

std::filesystem::path GetSongBackground(Game::Song &Song)
{
    auto SngDir = Song.SongDirectory;

    if (Song.BackgroundFilename.length() != 0 && std::filesystem::exists(SngDir / Song.BackgroundFilename))
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
    ImageList List;
    VSRG::Song* Song;
    VSRG::Difficulty* Difficulty;
    bool Validated;
    bool BlackToTransparent;
public:
    BMSBackground(Interruptible* parent, VSRG::Difficulty* Difficulty, VSRG::Song* Song) : BackgroundAnimation(parent), List(this)
    {
        this->Difficulty = Difficulty;
        this->Song = Song;
        Validated = false;
        MissTime = 0;

        bool BtoT = false;
        if (Difficulty->Data->TimingInfo->GetType() == VSRG::TI_BMS)
        {
            if (!std::dynamic_pointer_cast<VSRG::BMSTimingInfo>(Difficulty->Data->TimingInfo)->IsBMSON)
                BtoT = true;
        }
        BlackToTransparent = BtoT;
    }

    void Load() override
    {
        EventsLayer0 = Difficulty->Data->BMPEvents->BMPEventsLayerBase;
        EventsLayerMiss = Difficulty->Data->BMPEvents->BMPEventsLayerMiss;
        EventsLayer1 = Difficulty->Data->BMPEvents->BMPEventsLayer;
        EventsLayer2 = Difficulty->Data->BMPEvents->BMPEventsLayer2;

        for (auto v : Difficulty->Data->BMPEvents->BMPList)
            List.AddToListIndex(v.second, Song->SongDirectory, v.first);

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

        LayerMiss->SetImage(List.GetFromIndex(0), false);
        Layer0->SetImage(List.GetFromIndex(1), false);

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

        Transform.SetWidth(256);
        Transform.SetHeight(256);

        Validated = true;
    }

    void SetLayerImage(Sprite *sprite, std::vector<AutoplayBMP> &events_layer, double time)
    {
        auto bmp = std::lower_bound(events_layer.begin(), events_layer.end(), time);
        if (bmp != events_layer.begin())
        {
            bmp = bmp - 1;
            sprite->SetImage(List.GetFromIndex(bmp->BMP), false);
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
        Log::Printf("Using static background: %s\n", Filename.c_str());
        List.AddToListIndex(Filename, "", 0);
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

std::shared_ptr<BackgroundAnimation> CreateBGAforVSRG(VSRG::Song &input, uint8_t DifficultyIndex, Interruptible *context)
{
    VSRG::Difficulty* Diff;
    if (Diff = input.GetDifficulty(DifficultyIndex))
    {
        if (Diff->Data && Diff->Data->BMPEvents)
            return std::make_shared<BMSBackground>(context, Diff, &input);
        else
            return std::make_shared<StaticBackground>(context, GetSongBackground(input));
    }

    return nullptr;
}

std::shared_ptr<BackgroundAnimation> CreateBGAforDotcur(dotcur::Song &input, uint8_t DifficultyIndex)
{
    return std::make_shared<StaticBackground>(nullptr, GetSongBackground(input));
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

std::shared_ptr<BackgroundAnimation> BackgroundAnimation::CreateBGAFromSong(uint8_t DifficultyIndex, Game::Song& Input, Interruptible* context, bool LoadNow)
{
    std::shared_ptr<BackgroundAnimation> ret = nullptr;

    switch (Input.Mode)
    {
    case MODE_VSRG:
        ret = CreateBGAforVSRG(static_cast<VSRG::Song&> (Input), DifficultyIndex, context);
        break;
    case MODE_DOTCUR:
        ret = CreateBGAforDotcur(static_cast<dotcur::Song&> (Input), DifficultyIndex);
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