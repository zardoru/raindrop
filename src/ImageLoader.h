#pragma once

#include <map>
#include "Image.h"

class ImageLoader
{
private:

	struct UploadData
	{
		unsigned char* Data;
		int Width, Height;
	};

	static std::map<GString, Image*> Textures;
	static std::map<GString, UploadData> PendingUploads;

	static Image*		InsertImage(GString Name, ImageData *imgData);
public:
	
	ImageLoader();
	~ImageLoader();
	
	static void   InvalidateAll();
	static void   UnloadAll();

	static void   DeleteImage(Image* &ToDelete);

	/* For multi-threaded loading. */
	static void   AddToPending (const char* Filename);
	static void   LoadFromManifest(char** Manifest, int Count, GString Prefix = "");
	static void   UpdateTextures();
	static ImageData GetDataForImage(GString filename);
	static ImageData GetDataForImageFromMemory(const unsigned char *const buffer, size_t len);

	/* On-the-spot, main thread loading or reloading. */
	static Image* Load(GString filename);
};