#include <filesystem>
#include <map>
#include <rmath.h>


#include <game/GameConstants.h>
#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"

#include "Texture.h"
#include "ImageList.h"
#include "ImageLoader.h"

#include "Transformation.h"
#include "Rendering.h"
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

void ImageList::AddToList(const std::filesystem::path& Filename, const std::filesystem::path& Prefix)
{
    auto ResFilename = Prefix / Filename;

    if (Images.find(ResFilename) == Images.end())
    {
        ImageLoader::AddToPending(ResFilename);
        Images[ResFilename] = nullptr;
    }
}

void ImageList::AddToListIndex(const std::filesystem::path& Filename, int Index)
{
    if (ImagesIndex.find(Index) == ImagesIndex.end())
    {
        ImageLoader::AddToPending(Filename);
        Images[Filename] = nullptr;
        ImagesIndex[Index] = nullptr;
        ImagesIndexPending[Index] = Filename;
    }
}

void ImageList::AddToListIndex(Texture * tex, int Index)
{
	ImagesIndex[Index] = tex;
}

void ImageList::Destroy()
{
    for (auto & Image : Images)
        ImageLoader::DeleteImage(Image.second);
}

void ImageList::AddToList(const uint32_t Count, const std::string *Filename, const std::string& Prefix)
{
    for (uint32_t i = 0; i < Count; i++)
    {
        AddToList(Filename[i], Prefix);
    }
}

bool ImageList::LoadAll()
{
    bool WereErrors = false;
    for (auto & Image : Images)
    {
        if (Image.first.empty())
            continue;

        Image.second = ImageLoader::Load(Image.first);
        if (Image.second == nullptr)
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
Texture* ImageList::GetFromFilename(const std::string& Filename)
{
    return Images[Filename];
}

// Gets image from SkinPrefix + filename
Texture* ImageList::GetFromSkin(const std::string& Filename)
{
    return Images[GameState::GetInstance().GetSkinPrefix() + Filename];
}

Texture* ImageList::GetFromIndex(int Index)
{
    return ImagesIndex[Index];
}

void ImageList::ForceFetch()
{
    Sprite Fill;

    for (auto & Image : Images)
    {
        Fill.SetImage(Image.second, false);

        // Draw as black.
        Fill.Color.Red = Fill.Color.Blue = Fill.Color.Green = 0.0001f;
        Fill.Alpha = 0.0001f;
        Fill.Render();
    }
}