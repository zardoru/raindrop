#pragma once

#include "Texture2D.h"

class TextureCollection
{
private:

    struct UploadData
    {
        std::vector<uint32_t> Data;
        int Width, Height;
    };

    static std::map<std::filesystem::path, Texture2D*> textures_;
    static std::map<std::filesystem::path, UploadData> pending_uploads_;

    static Texture2D*		get_texture_2d_from_file(const std::filesystem::path& name, const ImageData2d &img_data);
public:

    TextureCollection();
    ~TextureCollection();

    static void   invalidate_all();
    static void   unload_all();

    static void   delete_texture_2d(Texture2D* &to_delete);

    /* For multi-threaded loading. */
    static void   add_to_pending_2d_uploads(const std::filesystem::path& filename);
    static void   load_from_manifest(const char** manifest, int count, const std::string& prefix = "");
    static void   upload_and_reload_textures();
    static ImageData2d get_data_for_image(std::filesystem::path filename);
    static ImageData2d get_data_for_image_from_memory(const unsigned char *const buffer, size_t len);
	static void	  reload_all();
	static void register_texture(Texture2D* tex);

    /* On-the-spot, main thread loading or reloading. */
    static Texture2D* load(const std::filesystem::path& filename);
};