#pragma once

// A front-end to ImageLoader that unloads all images added to the list on destruction.

#include "Interruptible.h"

class Texture2D;

/*
    In particular, allows a manifest of filenames to be passed to it and control when it loads those images.
*/
class ImageList : public Interruptible
{
    std::map <std::filesystem::path, Texture2D*> images_;
    std::map <int, std::filesystem::path> images_index_pending_;
    std::map <int, Texture2D*> images_index_;
    bool should_delete_at_destruction_;

public:
    explicit ImageList(bool release_at_destruction = true);

    explicit ImageList(Interruptible *parent, bool release_at_destruction = true);
    ~ImageList() override;

    void destroy();
    void add_to_list(const std::filesystem::path& filename, const std::filesystem::path& prefix);

	// AddToListIndex asks ImageLoader to load on a different thread.
    void add_to_list_index(const std::filesystem::path& filename, int index);

	// Add texture (Doesn't get removed)
	void add_to_list_index(Texture2D* tex, int index);

    void add_to_list(const uint32_t count, const std::string *filename, const std::string& prefix);

	// Either load from scratch or from cached image. Unsafe for non-main thread.
    bool load_all();

    void force_fetch() const;

    // Gets image from this filename
    Texture2D* get_from_filename(const std::string& filename);

    // Gets image from this index
    Texture2D* get_from_index(int index);

    // Gets image from SkinPrefix + filename
    Texture2D* get_from_skin(const std::string& filename);
};