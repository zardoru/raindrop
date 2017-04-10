#pragma once

struct ImageData
{
	std::filesystem::path Filename;
    int Width, Height, Alignment;
    std::vector<uint32_t> Data;
	
	// If you can't copy into Data, then fill this pointer instead.
	uint32_t* TempData;

    ImageData()
    {
		Alignment = 1;
        Width = 0; Height = 0;
		TempData = nullptr;
    }

	ImageData(int w, int h, void* data, int align = 1) {
		ImageData();
		Width = w; Height = h;
		TempData = (uint32_t*)data;
		Alignment = align;

	}
};

class Texture
{
    friend class ImageLoader;
    static Texture* LastBound;

    void Destroy();

protected:
	bool TextureAssigned;
    void CreateTexture();
public:
    Texture(unsigned int texture, int w, int h);
    Texture();
    ~Texture();

    void Bind();
    void LoadFile(std::filesystem::path Filename, bool Regenerate = false);
    void SetTextureData2D(ImageData &Data, bool Reassign = false);

    // Utilitarian
    static void ForceRebind();
    static void Unbind(); // Or, basically unbind.

    // Data
	std::filesystem::path fname;
    int w, h;
    unsigned int texture;
    bool IsValid;
};
