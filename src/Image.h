#ifndef Image_H
#define Image_H

struct ImageData {
	enum {
		WM_CLAMP_TO_EDGE,
		WM_REPEAT
	} WrapMode;

	enum {
		SM_LINEAR,
		SM_NEAREST,
		SM_MIPMAP
	} ScalingMode;

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
	void Assign(Directory Filename, bool Regenerate = false);
	void SetTextureData(ImageData *Data, bool Reassign = false);

	// Utilitarian
	static void ForceRebind();
	static void BindNull(); // Or, basically unbind.

	// Data
	GString fname;
	int w, h;
	unsigned int texture;
	bool IsValid;
};

#endif
