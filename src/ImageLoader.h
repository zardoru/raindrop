#ifndef ImageLoader_H
#define ImageLoader_H

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

	static std::map<String, Image*> Textures;
	static std::map<String, UploadData> PendingUploads;

	static unsigned int UploadToGPU(unsigned char* Data, unsigned int Width, unsigned int Height);
	static Image*		InsertImage(String Name, unsigned int Texture, int Width, int Height);
public:
	
	ImageLoader();
	~ImageLoader();
	
	static void   InvalidateAll();
	static void   UnloadAll();

	static void   DeleteImage(Image* &ToDelete);

	/* For multi-threaded loading. */
	static void   AddToPending (const char* Filename);
	static void   LoadFromManifest(char** Manifest, int Count, String Prefix = "");
	static void   UpdateTextures();

	/* On-the-spot, main thread loading or reloading. */
	static Image* Load(std::string filename);
	static Image* LoadSkin(std::string filename);
};

#endif