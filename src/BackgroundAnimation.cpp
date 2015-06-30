#include "GameGlobal.h"
#include "Sprite.h"
#include "Song.h"
#include "BackgroundAnimation.h"
#include "Song7K.h"
#include "SongDC.h"
#include "ImageLoader.h"
#include "ImageList.h"

Directory GetSongBackground(Game::Song &Song)
{
	vector<GString> DirCnt;
	Directory SngDir = Song.SongDirectory;

	if (Song.BackgroundFilename.length() != 0 && Utility::FileExists(SngDir / Song.BackgroundFilename))
		return SngDir / Song.BackgroundFilename;

	SngDir.ListDirectory(DirCnt, Directory::FS_REG);
	for (auto i: DirCnt)
	{
		GString ext = Directory(i).GetExtension();
		if (i.find("bg") && (ext == "jpg" || ext == "png"))
			return SngDir / i;
	}

	return "";
}

class BMSBackground : public BackgroundAnimation
{
	shared_ptr<Sprite> Layer0;
	shared_ptr<Sprite> LayerMiss;
	shared_ptr<Sprite> Layer1;
	shared_ptr<Sprite> Layer2;
	vector<AutoplayBMP> EventsLayer0;
	vector<AutoplayBMP> EventsLayerMiss;
	vector<AutoplayBMP> EventsLayer1;
	vector<AutoplayBMP> EventsLayer2;
	ImageList List;
	VSRG::Song* Song;
	VSRG::Difficulty* Difficulty;
	bool Validated;

public:
	BMSBackground(VSRG::Difficulty* Difficulty, VSRG::Song* Song)
	{
		this->Difficulty = Difficulty;
		this->Song = Song;
		Validated = false;
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

		Layer0 = make_shared<Sprite>();
		LayerMiss = make_shared<Sprite>();
		Layer1 = make_shared<Sprite>();
		Layer2 = make_shared<Sprite>();

		Layer0->ChainTransformation(this);
		LayerMiss->ChainTransformation(this);
		Layer1->ChainTransformation(this);
		Layer2->ChainTransformation(this);

		Layer0->SetImage(List.GetFromIndex(0), false);

		SetWidth(ScreenWidth);
		SetHeight(ScreenHeight);

		Validated = true;
	}

	void SetLayerImage(Sprite *sprite, vector<AutoplayBMP> &events_layer, double time)
	{
		auto bmp = std::lower_bound(events_layer.begin(), events_layer.end(), time);
		if (bmp != events_layer.end())
			sprite->SetImage(List.GetFromIndex(bmp->BMP), false);
	}

	void SetAnimationTime(double Time) override
	{
		if (!Validated) return;

		SetLayerImage(Layer0.get(), EventsLayer0, Time);
		SetLayerImage(LayerMiss.get(), EventsLayerMiss, Time);
		SetLayerImage(Layer1.get(), EventsLayer1, Time);
		SetLayerImage(Layer2.get(), EventsLayer2, Time);
	}

	void Render() override
	{
		Layer0->Render();
		LayerMiss->Render();
		Layer1->Render();
		Layer2->Render();
	}
};

class StaticBackground : public BackgroundAnimation
{
	shared_ptr<Sprite> Background;
	ImageList List;
public:
	StaticBackground(GString Filename)
		: List()
	{
		List.AddToListIndex(Filename, "", 0);
	}

	void SetAnimationTime(double Time) override {}

	void Validate() override
	{
		if (!Background)
		{
			Background = make_shared<Sprite>();
			Background->SetImage(List.GetFromIndex(0), false);
			Background->ChainTransformation(this);
			SetWidth(ScreenWidth);
			SetHeight(ScreenHeight);
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

shared_ptr<BackgroundAnimation> CreateBGAforVSRG(VSRG::Song &input, uint8_t DifficultyIndex)
{
	VSRG::Difficulty* Diff;
	if (Diff = input.GetDifficulty(DifficultyIndex))
	{
		if (Diff->Data && Diff->Data->BMPEvents)
			return make_shared<BMSBackground>(Diff, &input);
		else
			return make_shared<StaticBackground>(input.SongDirectory / input.BackgroundFilename);
	}

	return nullptr;
}

shared_ptr<BackgroundAnimation> CreateBGAforDotcur(dotcur::Song &input, uint8_t DifficultyIndex)
{
	return make_shared<StaticBackground>(input.BackgroundFilename);
}

void BackgroundAnimation::Load()
{
}

void BackgroundAnimation::Validate()
{
}

shared_ptr<BackgroundAnimation> BackgroundAnimation::CreateBGAFromSong(uint8_t DifficultyIndex, Game::Song& Input, bool LoadNow)
{
	shared_ptr<BackgroundAnimation> ret = nullptr;

	switch (Input.Mode)
	{
	case MODE_VSRG:
		ret = CreateBGAforVSRG(static_cast<VSRG::Song&> (Input), DifficultyIndex);
		break;
	case MODE_DOTCUR:
		ret = CreateBGAforDotcur(static_cast<dotcur::Song&> (Input), DifficultyIndex);
		break;
	default:
		break;
	}

	return ret;
}