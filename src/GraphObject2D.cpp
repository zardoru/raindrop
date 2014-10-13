#include "Global.h"
#include "GraphObject2D.h"

#include "VBO.h"
#include "Image.h"

bool GraphObject2D::IsInitialized = false;


GraphObject2D::GraphObject2D(bool ShouldInitTexture) 
{
	SetCropToWholeImage();
	mWidth = mHeight = 0;

	mPosition = Vec2(0, 0);
	SetScale(1);
	mRotation = 0;
	Lighten = false;
	LightenFactor = 1.0f;
	BlackToTransparent = false;

	BlendingMode = MODE_ALPHA;

	Red = Blue = Green = 1.0;
	Alpha = 1.0;
		
	z_order = 0;

	Centered = false;
	ColorInvert = false;
	DirtyMatrix = true;
	DirtyTexture = true;
	DoTextureCleanup = true;
	AffectedByLightning = false;

	mImage = NULL;
	UvBuffer = NULL;

	Initialize(ShouldInitTexture);
}

GraphObject2D::~GraphObject2D()
{
	Cleanup();
}

void GraphObject2D::SetBlendMode(rBlendMode Mode)
{
	BlendingMode = Mode;
}

void GraphObject2D::Initialize(bool ShouldInitTexture)
{
	UvBuffer = NULL;

	if (!IsInitialized)
	{
		mBuffer = new VBO(VBO::Static, 8);
		InitVBO();
		IsInitialized = true;
	}

	if (ShouldInitTexture)
		UpdateTexture();
}


void GraphObject2D::SetImage(Image* image, bool ChangeSize)
{
	mImage = image;
	if (image)
	{
		if (ChangeSize)
		{
			SetCropToWholeImage();
			mWidth = image->w;
			mHeight = image->h;
			DirtyMatrix = true;
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

// Scale
void GraphObject2D::SetScale(Vec2 Scale)
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

Vec2 GraphObject2D::GetScale() const
{
	return mScale;
}

// Position
void GraphObject2D::SetPosition(Vec2 Pos)
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

void GraphObject2D::AddPosition(Vec2 pos)
{
	mPosition += pos;
	DirtyMatrix = true;
}

Vec2 GraphObject2D::GetPosition() const
{
	return mPosition;
}

// Size
void GraphObject2D::SetSize(Vec2 Size)
{
	mWidth = Size.x;
	mHeight = Size.y;
	DirtyMatrix = true;
}

void GraphObject2D::SetSize(float Size)
{
	SetSize(Vec2(Size, Size));
}

void GraphObject2D::SetSize(float W, float H)
{
	SetSize(Vec2(W, H));
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

Vec2 GraphObject2D::GetSize() const
{
	return Vec2(mWidth, mHeight);
}

float GraphObject2D::GetWidth() const
{
	return mWidth;
}

float GraphObject2D::GetHeight() const
{
	return mHeight;
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

	if (mRotation >= 360)
		mRotation -= 360;
	DirtyMatrix = true;
}

void GraphObject2D::Invalidate()
{
	IsInitialized = false;
}

uint32 GraphObject2D::GetZ() const
{
	return z_order;
}

void GraphObject2D::SetZ(uint32 Z)
{
	z_order = Z;
	DirtyMatrix = true;
}

Image* GraphObject2D::GetImage()
{
	return mImage;
}

void GraphObject2D::BindTopLeftVBO()
{
	mBuffer->Bind();
}

void GraphObject2D::BindTextureVBO()
{
	UvBuffer->Bind();
}

const glm::mat4 &GraphObject2D::GetMatrix()
{
	UpdateMatrix();
	return Matrix;
}

bool GraphObject2D::ShouldUpdateMatrix() const
{
	return DirtyMatrix;
}