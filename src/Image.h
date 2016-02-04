#pragma once

struct ImageData
{
    enum EWrapMode
    {
        WM_CLAMP_TO_EDGE,
        WM_REPEAT,
        WM_DEFAULT = WM_CLAMP_TO_EDGE
    } WrapMode;

    enum EScalingMode
    {
        SM_LINEAR,
        SM_NEAREST,
        SM_MIPMAP,
        SM_DEFAULT = SM_MIPMAP
    } ScalingMode;

    std::string Filename;
    int Width, Height;
    void *Data;

    ImageData()
    {
        WrapMode = WM_DEFAULT;
        ScalingMode = SM_DEFAULT;
        Width = 0; Height = 0;
        Data = nullptr;
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
    void Assign(Directory Filename,
        ImageData::EScalingMode ScaleMode = ImageData::SM_DEFAULT,
        ImageData::EWrapMode WrapMode = ImageData::WM_DEFAULT,
        bool Regenerate = false);
    void SetTextureData(ImageData *Data, bool Reassign = false);

    // Utilitarian
    static void ForceRebind();
    static void BindNull(); // Or, basically unbind.

    // Data
    std::string fname;
    int w, h;
    unsigned int texture;
    bool IsValid;
};
