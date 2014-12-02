#ifndef Image_H
#define Image_H

struct ImageData {
	GString Filename;
	int Width, Height;
	unsigned char* Data;
};

class Image
{
	friend class ImageLoader;
	static Image* LastBound;
	bool TextureAssigned;

	void Destroy();
	void CreateTexture();
public:
	Image(unsigned int texture, int w, int h);
	Image();
	~Image();
	
	void Bind();
	void Assign(Directory Filename);
	void SetTextureData(ImageData *Data);

	// Utilitarian
	static void ForceRebind();

	// Data
	GString fname;
	int w, h;
	unsigned int texture;
	bool IsValid;
};

#endif
