#pragma once

// A front-end to ImageLoader that unloads all images added to the list on destruction.

#include "Interruptible.h"

/*
    In particular, allows a manifest of filenames to be passed to it and control when it loads those images.
*/
class ImageList : public Interruptible
{
    std::map <std::string, Image*> Images;
    std::map <int, std::string> ImagesIndexPending;
    std::map <int, Image*> ImagesIndex;
    bool ShouldDeleteAtDestruction;

public:

    ImageList(bool ReleaseAtDestruction = true);
    ImageList(Interruptible *parent, bool ReleaseAtDestruction = true);
    ~ImageList();

    void Destroy();
    void AddToList(const std::string Filename, const std::string Prefix);
    void AddToListIndex(const std::string Filename, const std::string Prefix, int Index);
    void AddToList(const uint32_t Count, const std::string *Filename, const std::string Prefix);
    bool LoadAll();

    void ForceFetch();

    // Gets image from this filename
    Image* GetFromFilename(const std::string Filename);

    // Gets image from this index
    Image* GetFromIndex(int Index);

    // Gets image from SkinPrefix + filename
    Image* GetFromSkin(const std::string Filename);
};