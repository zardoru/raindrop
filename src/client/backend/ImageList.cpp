#include <filesystem>
#include <map>
#include <rmath.h>


#include "../game/GameState.h"

#include "Texture2D.h"
#include "ImageList.h"

#include <ranges>

#include "TextureCollection.h"

#include "Sprite.h"

ImageList::ImageList(const bool release_at_destruction)
{
    should_delete_at_destruction_ = release_at_destruction;
}

ImageList::ImageList(Interruptible *Parent, const bool release_at_destruction)
    : Interruptible(Parent)
{
    should_delete_at_destruction_ = release_at_destruction;
}

ImageList::~ImageList()
{
    if (!should_delete_at_destruction_)
        return;

    destroy();
}

void ImageList::add_to_list(const std::filesystem::path& filename, const std::filesystem::path& prefix)
{
    auto ResFilename = prefix / filename;

    if (images_.find(ResFilename) == images_.end())
    {
        TextureCollection::add_to_pending_2d_uploads(ResFilename);
        images_[ResFilename] = nullptr;
    }
}

void ImageList::add_to_list_index(const std::filesystem::path& filename, const int index)
{
    if (images_index_.find(index) == images_index_.end())
    {
        TextureCollection::add_to_pending_2d_uploads(filename);
        images_[filename] = nullptr;
        images_index_[index] = nullptr;
        images_index_pending_[index] = filename;
    }
}

void ImageList::add_to_list_index(Texture2D * tex, const int index)
{
	images_index_[index] = tex;
}

void ImageList::destroy()
{
    for (auto & Image : images_)
        TextureCollection::delete_texture_2d(Image.second);
}

void ImageList::add_to_list(const uint32_t count, const std::string *filename, const std::string& prefix)
{
    for (uint32_t i = 0; i < count; i++)
    {
        add_to_list(filename[i], prefix);
    }
}

bool ImageList::load_all()
{
    bool WereErrors = false;
    for (auto & Image : images_)
    {
        if (Image.first.empty())
            continue;

        Image.second = TextureCollection::load(Image.first);
        if (Image.second == nullptr)
            WereErrors = true;
        CheckInterruption();
    }

    for (auto i = images_index_pending_.begin(); i != images_index_pending_.end();)
    {
        images_index_[i->first] = TextureCollection::load(i->second);
        if (images_index_[i->first] == nullptr)
            WereErrors = true;

        i = images_index_pending_.erase(i);
        CheckInterruption();
    }

    return WereErrors;
}

// Gets image from this filename
Texture2D* ImageList::get_from_filename(const std::string& filename)
{
    return images_[filename];
}

// Gets image from SkinPrefix + filename
Texture2D* ImageList::get_from_skin(const std::string& filename)
{
    return images_[GameState::get_instance().get_skin_prefix() + filename];
}

Texture2D* ImageList::get_from_index(const int index)
{
    return images_index_[index];
}

void ImageList::force_fetch() const {
    Sprite fill;
    DrawCallSink calls;

    for (const auto &val: images_ | std::views::values)
    {
        fill.set_image(val, false);

        // Draw as black.
        fill.color.red = fill.color.blue = fill.color.green = 0.0001f;
        fill.color.alpha = 0.0001f;
        fill.emit_draw_calls(calls);
    }

    calls.flush();
}
