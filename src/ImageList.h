#pragma once

// A front-end to ImageLoader that unloads all images added to the list on destruction.

#include "Interruptible.h"

class Texture;

/*
    In particular, allows a manifest of filenames to be passed to it and control when it loads those images.
*/
class ImageList : public Interruptible
{
    std::map <std::filesystem::path, Texture*> Images;
    std::map <int, std::filesystem::path> ImagesIndexPending;
    std::map <int, Texture*> ImagesIndex;
    bool ShouldDeleteAtDestruction;

public:

    ImageList(bool ReleaseAtDestruction = true);
    ImageList(Interruptible *parent, bool ReleaseAtDestruction = true);
    ~ImageList();

    void Destroy();
    void AddToList(const std::filesystem::path Filename, const std::filesystem::path Prefix);

	// AddToListIndex asks ImageLoader to load on a different thread.
    void AddToListIndex(const std::filesystem::path Filename, int Index);

	// Add texture (Doesn't get removed)
	void AddToListIndex(Texture* tex, int Index);

    void AddToList(const uint32_t Count, const std::string *Filename, const std::string Prefix);

	// Either load from scratch or from cached image. Unsafe for non-main thread.
    bool LoadAll();

    void ForceFetch();

    // Gets image from this filename
    Texture* GetFromFilename(const std::string Filename);

    // Gets image from this index
    Texture* GetFromIndex(int Index);

    // Gets image from SkinPrefix + filename
    Texture* GetFromSkin(const std::string Filename);
};