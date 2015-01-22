#include "Global.h"
#include "GraphObject2D.h"

#include "VBO.h"
#include "Image.h"

GraphObject2D::GraphObject2D(bool ShouldInitTexture) : Drawable2D()
{
	Construct(ShouldInitTexture);
}

GraphObject2D::GraphObject2D() : Drawable2D()
{
	Construct(true);
}

void GraphObject2D::Construct(bool doInitTexture)
{
	SetCropToWholeImage();
	
	Lighten = false;
	LightenFactor = 1.0f;
	BlackToTransparent = false;

	BlendingMode = MODE_ALPHA;

	Red = Blue = Green = 1.0;
	Alpha = 1.0;

	Centered = false;
	ColorInvert = false;
	DirtyTexture = true;
	DoTextureCleanup = doInitTexture;
	AffectedByLightning = false;

	mImage = NULL;
	UvBuffer = NULL;

	Initialize(doInitTexture);
}

void GraphObject2D::Initialize(bool ShouldInitTexture)
{
	UvBuffer = NULL;

	if (ShouldInitTexture)
		UpdateTexture();
}

GraphObject2D::~GraphObject2D()
{
	Cleanup();
}

void GraphObject2D::SetBlendMode(int Mode)
{
	BlendingMode = (RBlendMode)Mode;
}

int GraphObject2D::GetBlendMode() const
{
	return BlendingMode;
}

void GraphObject2D::SetImage(Image* image, bool ChangeSize)
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

void GraphObject2D::SetCropByPixels(int32 x1, int32 x2, int32 y1, int32 y2)
{
	if (mImage)
	{
		mCrop_x1 = (float) x1 / (float) mImage->w;
		mCrop_x2 = (float) x2 / (float) mImage->w;
		mCrop_y1 = (float) y1 / (float) mImage->h;
		mCrop_y2 = (float) y2 / (float) mImage->h;

		DirtyTexture = true;
		UpdateTexture();
	}
}

void GraphObject2D::SetCropToWholeImage()
{
	mCrop_x1 = 0;
	mCrop_x2 = 1;
	mCrop_y1 = 0;
	mCrop_y2 = 1;
	DirtyTexture = true;
}

void GraphObject2D::SetCrop(Vec2 Crop1, Vec2 Crop2)
{
	mCrop_x1 = Crop1.x;
	mCrop_y1 = Crop1.y;
	mCrop_x2 = Crop2.x;
	mCrop_y2 = Crop2.y;
	DirtyTexture = true;
}

void GraphObject2D::SetCrop1(Vec2 Crop1)
{
	mCrop_x1 = Crop1.x;
	mCrop_y1 = Crop1.y;
	DirtyTexture = true;
}

void GraphObject2D::SetCrop2(Vec2 Crop2)
{
	mCrop_x2 = Crop2.x;
	mCrop_y2 = Crop2.y;
	DirtyTexture = true;
}


void GraphObject2D::Invalidate()
{
	// stub
}

Image* GraphObject2D::GetImage()
{
	return mImage;
}

void GraphObject2D::BindTextureVBO()
{
	UvBuffer->Bind();
}

GString GraphObject2D::GetImageFilename() const
{
	if (mImage)
		return mImage->fname;
	else
		return GString();
}
