#pragma once

#include "Texture.h"

class ImageLoader
{
private:

    struct UploadData
    {
        std::vector<uint32_t> Data;
        int Width, Height;
    };

    static std::map<std::filesystem::path, Texture*> Textures;
    static std::map<std::filesystem::path, UploadData> PendingUploads;

    static Texture*		InsertImage(std::filesystem::path Name, ImageData &imgData);
public:

    ImageLoader();
    ~ImageLoader();

    static void   InvalidateAll();
    static void   UnloadAll();

    static void   DeleteImage(Texture* &ToDelete);

    /* For multi-threaded loading. */
    static void   AddToPending(std::filesystem::path Filename);
    static void   LoadFromManifest(const char** Manifest, int Count, std::string Prefix = "");
    static void   UpdateTextures();
    static ImageData GetDataForImage(std::filesystem::path filename);
    static ImageData GetDataForImageFromMemory(const unsigned char *const buffer, size_t len);
	static void	  ReloadAll();
	static void RegisterTexture(Texture* tex);

    /* On-the-spot, main thread loading or reloading. */
    static Texture* Load(std::filesystem::path filename);
};