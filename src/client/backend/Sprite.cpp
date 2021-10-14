#include <string>
#include <TextAndFileUtil.h>
#include <cstdint>
#include <rmath.h>

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "VBO.h"
#include "Texture.h"

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

    Color.Red = Color.Blue = Color.Green = 1.0;
    Alpha = 1.0;

    Centered = false;
    ColorInvert = false;
    DirtyTexture = true;
    DoTextureCleanup = doInitTexture;
    AffectedByLightning = false;

    mTexture = nullptr;
    UvBuffer = nullptr;
	mShader = nullptr;

    Scissor = false;
    ScissorRegion = AABB ();

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

void Sprite::SetShader(Renderer::Shader * s)
{
	mShader = s;
}

Renderer::Shader * Sprite::GetShader() const
{
	return mShader;
}

void Sprite::SetImage(Texture* image, bool ChangeSize)
{
    if (mTexture != image)
    {
        mTexture = image;
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
    if (mTexture)
    {
        mCrop_x1 = (float)x1 / (float)mTexture->w;
        mCrop_x2 = (float)x2 / (float)mTexture->w;
        mCrop_y1 = (float)y1 / (float)mTexture->h;
        mCrop_y2 = (float)y2 / (float)mTexture->h;

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

Texture* Sprite::GetImage()
{
    return mTexture;
}

void Sprite::BindTextureVBO()
{
    UvBuffer->Bind();
}

std::string Sprite::GetImageFilename() const
{
    if (mTexture)
        return Conversion::ToU8(mTexture->fname.wstring());
    else
        return std::string();
}
