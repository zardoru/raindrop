#include "pch.h"

#include "Sprite.h"

#include "VBO.h"
#include "Image.h"

void Drawable2D::Render() {}

Sprite::Sprite(bool ShouldInitTexture) : Drawable2D()
{
    Construct(ShouldInitTexture);
}

Sprite::Sprite() : Drawable2D()
{
    Construct(true);
}

void Sprite::Construct(bool doInitTexture)
{
    SetCropToWholeImage();

    Lighten = false;
    LightenFactor = 1.0f;
    BlackToTransparent = false;

    BlendingMode = BLEND_ALPHA;

    Red = Blue = Green = 1.0;
    Alpha = 1.0;

    Centered = false;
    ColorInvert = false;
    DirtyTexture = true;
    DoTextureCleanup = doInitTexture;
    AffectedByLightning = false;

    mImage = nullptr;
    UvBuffer = nullptr;

    Initialize(doInitTexture);
}

void Sprite::Initialize(bool ShouldInitTexture)
{
    UvBuffer = nullptr;

	if (ShouldInitTexture)
		UpdateTexture();
	else
		UvBuffer = Renderer::GetDefaultTextureBuffer();
}

Sprite::~Sprite()
{
    Cleanup();
}

void Sprite::SetBlendMode(int Mode)
{
    BlendingMode = (EBlendMode)Mode;
}

int Sprite::GetBlendMode() const
{
    return BlendingMode;
}

void Sprite::SetImage(Image* image, bool ChangeSize)
{
    if (mImage != image)
    {
        mImage = image;
        if (image)
        {
            if (ChangeSize)
            {
                SetCropToWholeImage();
                SetSize(image->w, image->h);
            }
        }
    }
}

void Sprite::SetCropByPixels(int32_t x1, int32_t x2, int32_t y1, int32_t y2)
{
    if (mImage)
    {
        mCrop_x1 = (float)x1 / (float)mImage->w;
        mCrop_x2 = (float)x2 / (float)mImage->w;
        mCrop_y1 = (float)y1 / (float)mImage->h;
        mCrop_y2 = (float)y2 / (float)mImage->h;

        DirtyTexture = true;
        UpdateTexture();
    }
}

void Sprite::SetCropToWholeImage()
{
    mCrop_x1 = 0;
    mCrop_x2 = 1;
    mCrop_y1 = 0;
    mCrop_y2 = 1;
    DirtyTexture = true;
}

void Sprite::SetCrop(Vec2 Crop1, Vec2 Crop2)
{
    mCrop_x1 = Crop1.x;
    mCrop_y1 = Crop1.y;
    mCrop_x2 = Crop2.x;
    mCrop_y2 = Crop2.y;
    DirtyTexture = true;
}

void Sprite::SetCrop1(Vec2 Crop1)
{
    mCrop_x1 = Crop1.x;
    mCrop_y1 = Crop1.y;
    DirtyTexture = true;
}

void Sprite::SetCrop2(Vec2 Crop2)
{
    mCrop_x2 = Crop2.x;
    mCrop_y2 = Crop2.y;
    DirtyTexture = true;
}

void Sprite::Invalidate()
{
    // stub
}

Image* Sprite::GetImage()
{
    return mImage;
}

void Sprite::BindTextureVBO()
{
    UvBuffer->Bind();
}

std::string Sprite::GetImageFilename() const
{
    if (mImage)
        return Utility::ToU8(mImage->fname.wstring());
    else
        return std::string();
}
