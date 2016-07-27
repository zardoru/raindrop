#pragma once

struct ImageData
{
	std::filesystem::path Filename;
    int Width, Height;
    std::vector<uint32_t> Data;

    ImageData()
    {
        Width = 0; Height = 0;
    }
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
    void Assign(std::filesystem::path Filename, bool Regenerate = false);
    void SetTextureData(ImageData &Data, bool Reassign = false);

    // Utilitarian
    static void ForceRebind();
    static void BindNull(); // Or, basically unbind.

    // Data
	std::filesystem::path fname;
    int w, h;
    unsigned int texture;
    bool IsValid;
};
