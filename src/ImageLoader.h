#pragma once

#include "Image.h"

class ImageLoader
{
private:

	struct UploadData
	{
		void *Data;
		int Width, Height;
	};

	static std::map<std::string, Image*> Textures;
	static std::map<std::string, UploadData> PendingUploads;

	static Image*		InsertImage(std::string Name, ImageData *imgData);
public:
	
	ImageLoader();
	~ImageLoader();
	
	static void   InvalidateAll();
	static void   UnloadAll();

	static void   DeleteImage(Image* &ToDelete);

	/* For multi-threaded loading. */
	static void   AddToPending (const char* Filename);
	static void   LoadFromManifest(char** Manifest, int Count, std::string Prefix = "");
	static void   UpdateTextures();
	static ImageData GetDataForImage(std::string filename);
	static ImageData GetDataForImageFromMemory(const unsigned char *const buffer, size_t len);

	/* On-the-spot, main thread loading or reloading. */
	static Image* Load(std::string filename);
};