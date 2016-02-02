#include "pch.h"


#include "GameState.h"
#include "Image.h"
#include "ImageList.h"
#include "ImageLoader.h"
#include "Sprite.h"

ImageList::ImageList(bool ReleaseAtDestruction)
{
	ShouldDeleteAtDestruction = ReleaseAtDestruction;
}

ImageList::ImageList(Interruptible *Parent, bool ReleaseAtDestruction)
	: Interruptible(Parent)
{
	ShouldDeleteAtDestruction = ReleaseAtDestruction;
}

ImageList::~ImageList()
{
	if (!ShouldDeleteAtDestruction)
		return;

	Destroy();
}

void ImageList::AddToList(const std::string Filename, const std::string Prefix)
{
	Directory ResFilename = Directory(Prefix) / Filename;
	ResFilename.Normalize(true);

	if (Images.find(ResFilename) == Images.end())
	{
		ImageLoader::AddToPending(ResFilename.c_path());
		Images[ResFilename] = nullptr;
	}
}

void ImageList::AddToListIndex(const std::string Filename, const std::string Prefix, int Index)
{
	Directory ResFilename = Directory(Prefix) / Filename;

	ResFilename.Normalize(true);

	if (ImagesIndex.find(Index) == ImagesIndex.end())
	{
		ImageLoader::AddToPending(ResFilename.c_path());
		Images[ResFilename] = nullptr;
		ImagesIndex[Index] = nullptr;
		ImagesIndexPending[Index] = ResFilename;
	}

}

void ImageList::Destroy()
{
	for (auto i = Images.begin(); i != Images.end(); ++i)
		ImageLoader::DeleteImage(i->second);
}

void ImageList::AddToList(const uint32_t Count, const std::string *Filename, const std::string Prefix)
{
	for (uint32_t i = 0; i < Count; i++)
	{
		AddToList(Filename[i], Prefix);
	}
}

bool ImageList::LoadAll()
{
	bool WereErrors = false;
	for (auto i = Images.begin(); i != Images.end(); ++i)
	{
		i->second = ImageLoader::Load(i->first);
		if (i->second == nullptr)
			WereErrors = true;
		CheckInterruption();
	}

	for (auto i = ImagesIndexPending.begin(); i != ImagesIndexPending.end();)
	{
		ImagesIndex[i->first] = ImageLoader::Load(i->second);
		if (ImagesIndex[i->first] == nullptr)
			WereErrors = true;

		i = ImagesIndexPending.erase(i);
		CheckInterruption();
	}

	return WereErrors;
}

// Gets image from this filename
Image* ImageList::GetFromFilename(const std::string Filename)
{
	return Images[Filename];
}

// Gets image from SkinPrefix + filename
Image* ImageList::GetFromSkin(const std::string Filename)
{
	return Images[GameState::GetInstance().GetSkinPrefix() + Filename];
}

Image* ImageList::GetFromIndex(int Index)
{
	return ImagesIndex[Index];
}

void ImageList::ForceFetch()
{
	Sprite Fill;

	for (auto i = Images.begin(); i != Images.end(); ++i)
	{
		Fill.SetImage (i->second, false);

		// Draw as black.
		Fill.Red = Fill.Blue = Fill.Green = 0;
		Fill.Alpha = 0.0001f;
		Fill.Render();
	}
}