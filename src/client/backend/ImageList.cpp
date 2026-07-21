#include <filesystem>
#include <map>
#include <rmath.h>


#include "../game/GameState.h"

#include "Texture2D.h"
#include "ImageList.h"
#include "TextureCollection.h"

#include "Sprite.h"

ImageList::ImageList(const bool ReleaseAtDestruction)
{
    ShouldDeleteAtDestruction = ReleaseAtDestruction;
}

ImageList::ImageList(Interruptible *Parent, const bool ReleaseAtDestruction)
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
        TextureCollection::add_to_pending_2d_uploads(ResFilename);
        Images[ResFilename] = nullptr;
    }
}

void ImageList::AddToListIndex(const std::filesystem::path& Filename, const int Index)
{
    if (ImagesIndex.find(Index) == ImagesIndex.end())
    {
        TextureCollection::add_to_pending_2d_uploads(Filename);
        Images[Filename] = nullptr;
        ImagesIndex[Index] = nullptr;
        ImagesIndexPending[Index] = Filename;
    }
}

void ImageList::AddToListIndex(Texture2D * tex, const int Index)
{
	ImagesIndex[Index] = tex;
}

void ImageList::Destroy()
{
    for (auto & Image : Images)
        TextureCollection::delete_texture_2d(Image.second);
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

        Image.second = TextureCollection::load(Image.first);
        if (Image.second == nullptr)
            WereErrors = true;
        CheckInterruption();
    }

    for (auto i = ImagesIndexPending.begin(); i != ImagesIndexPending.end();)
    {
        ImagesIndex[i->first] = TextureCollection::load(i->second);
        if (ImagesIndex[i->first] == nullptr)
            WereErrors = true;

        i = ImagesIndexPending.erase(i);
        CheckInterruption();
    }

    return WereErrors;
}

// Gets image from this filename
Texture2D* ImageList::GetFromFilename(const std::string& Filename)
{
    return Images[Filename];
}

// Gets image from SkinPrefix + filename
Texture2D* ImageList::GetFromSkin(const std::string& Filename)
{
    return Images[GameState::get_instance().get_skin_prefix() + Filename];
}

Texture2D* ImageList::GetFromIndex(const int Index)
{
    return ImagesIndex[Index];
}

void ImageList::ForceFetch()
{
    Sprite Fill;
    DrawCallSink calls;

    for (auto & Image : Images)
    {
        Fill.set_image(Image.second, false);

        // Draw as black.
        Fill.color.red = Fill.color.blue = Fill.color.green = 0.0001f;
        Fill.color.alpha = 0.0001f;
        Fill.emit_draw_calls(calls);
    }

    calls.flush();
}
