#include "Global.h"
#include "GraphObject2D.h"

#include "VBO.h"
#include "Image.h"

#ifndef OLD_GL
bool GraphObject2D::IsInitialized = false;
#endif

GraphObject2D::GraphObject2D(bool ShouldInitTexture)
{
	SetCropToWholeImage();
	mWidth = mHeight = 0;

	mPosition = glm::vec2(0, 0);
	SetScale(1);
	mRotation = 0;

	Red = Blue = Green = 1.0;
	Alpha = 1.0;
		
	Centered = false;
	ColorInvert = false;
	DirtyMatrix = true;
	DoTextureCleanup = true;
	AffectedByLightning = false;

	mImage = NULL;

	Initialize(ShouldInitTexture);
}

GraphObject2D::~GraphObject2D()
{
	Cleanup();
}

void GraphObject2D::Initialize(bool ShouldInitTexture)
{
#ifndef OLD_GL
	UvBuffer = NULL;

	if (!IsInitialized)
	{
		mBuffer = new VBO(VBO::Static);
		mCenteredBuffer = new VBO(VBO::Static);
		InitVBO();
		IsInitialized = true;
	}

	if (ShouldInitTexture)
		UpdateTexture();
#endif
}


void GraphObject2D::SetImage(Image* image)
{
	if (image)
	{
		mImage = image;
		SetCropToWholeImage();
		mWidth = image->w;
		mHeight = image->h;
		DirtyMatrix = true;
	}
}

void GraphObject2D::SetCropByPixels(int32 x1, int32 x2, int32 y1, int32 y2)
{
	mCrop_x1 = (float) x1 / (float) mWidth;
	mCrop_x2 = (float) x2 / (float) mWidth;
	mCrop_y1 = (float) y1 / (float) mHeight;
	mCrop_y2 = (float) y2 / (float) mHeight;
	UpdateTexture();
}

void GraphObject2D::SetCropToWholeImage()
{
	mCrop_x1 = 0;
	mCrop_x2 = 1;
	mCrop_y1 = 0;
	mCrop_y2 = 1;
}

// Scale
void GraphObject2D::SetScale(glm::vec2 Scale)
{
	mScale = Scale;
	DirtyMatrix = true;
}

void GraphObject2D::SetScale(float Scale)
{
	SetScaleX(Scale);
	SetScaleY(Scale);
}

void GraphObject2D::SetScaleX(float ScaleX)
{
	mScale.x = ScaleX;
	DirtyMatrix = true;
}

void GraphObject2D::SetScaleY(float ScaleY)
{
	mScale.y = ScaleY;
	DirtyMatrix = true;
}

glm::vec2 GraphObject2D::GetScale() const
{
	return mScale;
}

// Position
void GraphObject2D::SetPosition(glm::vec2 Pos)
{
	mPosition = Pos;
	DirtyMatrix = true;
}

void GraphObject2D::SetPosition(float pX, float pY)
{
	SetPositionX(pX);
	SetPositionY(pY);
}

void GraphObject2D::SetPositionX(float pX)
{
	mPosition.x = pX;
	DirtyMatrix = true;
}

void GraphObject2D::SetPositionY(float pY)
{
	mPosition.y = pY;
	DirtyMatrix = true;
}

void GraphObject2D::AddPosition(float pX, float pY)
{
	mPosition.x += pX;
	mPosition.y += pY;
	DirtyMatrix = true;
}

void GraphObject2D::AddPosition(glm::vec2 pos)
{
	mPosition += pos;
	DirtyMatrix = true;
}

glm::vec2 GraphObject2D::GetPosition() const
{
	return mPosition;
}

// Size
void GraphObject2D::SetSize(glm::vec2 Size)
{
	mWidth = Size.x;
	mHeight = Size.y;
	DirtyMatrix = true;
}

void GraphObject2D::SetSize(float Size)
{
	SetSize(glm::vec2(Size, Size));
}

void GraphObject2D::SetSize(float W, float H)
{
	SetSize(glm::vec2(W, H));
}

void GraphObject2D::SetWidth(uint32 W)
{
	mWidth = W;
	DirtyMatrix = true;
}

void GraphObject2D::SetHeight(uint32 H)
{
	mHeight = H;
	DirtyMatrix = true;
}

glm::vec2 GraphObject2D::GetSize() const
{
	return glm::vec2(mWidth, mHeight);
}

float GraphObject2D::GetWidth() const
{
	return mWidth;
}

float GraphObject2D::GetHeight() const
{
	return mHeight;
}

void GraphObject2D::SetCrop(glm::vec2 Crop1, glm::vec2 Crop2)
{
	mCrop_x1 = Crop1.x;
	mCrop_y1 = Crop1.y;
	mCrop_x2 = Crop2.x;
	mCrop_y2 = Crop2.y;
	DirtyTexture = true;
}

void GraphObject2D::SetCrop1(glm::vec2 Crop1)
{
	mCrop_x1 = Crop1.x;
	mCrop_y1 = Crop1.y;
	DirtyTexture = true;
}

void GraphObject2D::SetCrop2(glm::vec2 Crop2)
{
	mCrop_x2 = Crop2.x;
	mCrop_y2 = Crop2.y;
	DirtyTexture = true;
}

// Rotation
void GraphObject2D::SetRotation(float Rot)
{
	mRotation = Rot;
	DirtyMatrix = true;
}


float GraphObject2D::GetRotation() const
{
	return mRotation;
}

void GraphObject2D::AddRotation(float Rot)
{
	mRotation += Rot;
	DirtyMatrix = true;
}

void GraphObject2D::Invalidate()
{
#ifndef OLD_GL
	IsInitialized = false;
#endif
}

Image* GraphObject2D::GetImage()
{
	return mImage;
}