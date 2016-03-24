#include "pch.h"

#include "Transformation.h"
//#include <glm/gtc/matrix_transform.inl>

bool Transformation::IsMatrixDirty()
{
	return mDirtyMatrix || (Chain && Chain->IsMatrixDirty());
}

Transformation::Transformation()
{
    SetSize(1);
    SetScale(1);
    SetRotation(0);
    SetPosition(0, 0);
    Chain = NULL;
    mLayer = 0;
	mDirtyMatrix = true;

    UpdateMatrix();
}

// Scale
void Transformation::SetScale(Vec2 Scale)
{
    mScale = Scale;
    mDirtyMatrix = true;
}

void Transformation::SetScale(float Scale)
{
    SetScaleX(Scale);
    SetScaleY(Scale);
}

void Transformation::SetScaleX(float ScaleX)
{
    mScale.x = ScaleX;
    mDirtyMatrix = true;
}

void Transformation::SetScaleY(float ScaleY)
{
    mScale.y = ScaleY;
    mDirtyMatrix = true;
}

Vec2 Transformation::GetScale() const
{
    return mScale;
}

// Position
void Transformation::SetPosition(Vec2 Pos)
{
    mPosition = Pos;
    mDirtyMatrix = true;
}

void Transformation::SetPosition(float pX, float pY)
{
    SetPositionX(pX);
    SetPositionY(pY);
}

void Transformation::SetPositionX(float pX)
{
    mPosition.x = pX;
    mDirtyMatrix = true;
}

void Transformation::SetPositionY(float pY)
{
    mPosition.y = pY;
    mDirtyMatrix = true;
}

void Transformation::AddPosition(float pX, float pY)
{
    mPosition.x += pX;
    mPosition.y += pY;
    mDirtyMatrix = true;
}

void Transformation::AddPosition(Vec2 pos)
{
    mPosition += pos;
    mDirtyMatrix = true;
}

void Transformation::AddPositionX(float pX)
{
    AddPosition(pX, 0);
}

void Transformation::AddPositionY(float pY)
{
    AddPosition(0, pY);
}

Vec2 Transformation::GetPosition() const
{
    return mPosition;
}

// Size
void Transformation::SetSize(Vec2 Size)
{
    mWidth = Size.x;
    mHeight = Size.y;
    mDirtyMatrix = true;
}

void Transformation::SetSize(float Size)
{
    SetSize(Vec2(Size, Size));
}

void Transformation::SetSize(float W, float H)
{
    SetSize(Vec2(W, H));
}

void Transformation::SetWidth(float W)
{
    mWidth = W;
    mDirtyMatrix = true;
}

void Transformation::SetHeight(float H)
{
    mHeight = H;
    mDirtyMatrix = true;
}

Vec2 Transformation::GetSize() const
{
    return Vec2(mWidth, mHeight);
}

float Transformation::GetWidth() const
{
    return mWidth;
}

float Transformation::GetHeight() const
{
    return mHeight;
}

// Rotation
void Transformation::SetRotation(float Rot)
{
    mRotation = Rot;
    mDirtyMatrix = true;
}

float Transformation::GetRotation() const
{
    return mRotation;
}

void Transformation::AddRotation(float Rot)
{
    mRotation += Rot;

    if (mRotation >= 360)
        mRotation -= 360;
    mDirtyMatrix = true;
}

uint32_t Transformation::GetZ() const
{
    return mLayer;
}

void Transformation::SetZ(uint32_t Z)
{
    mLayer = Z;
    mDirtyMatrix = true;
}

const glm::mat4 &Transformation::GetMatrix()
{
    if (IsMatrixDirty())
        UpdateMatrix();

    return mMatrix;
}

float Transformation::GetScaleX() const
{
    return mScale.x;
}

float Transformation::GetScaleY() const
{
    return mScale.y;
}

float Transformation::GetPositionX() const
{
    return mPosition.x;
}

float Transformation::GetPositionY() const
{
    return mPosition.y;
}

void Transformation::UpdateMatrix()
{
    Mat4 Scl = glm::scale(Mat4(), glm::vec3(mWidth*mScale.x, mHeight*mScale.y, 1));
    Mat4 Pos = glm::translate(Mat4(), glm::vec3(mPosition.x, mPosition.y, mLayer));
    Mat4 Rot = glm::rotate(Mat4(), mRotation, glm::vec3(0, 0, 1));
    Mat4 Chn;

    if (Chain)
        Chn = Chain->GetMatrix();

    mMatrix = Chn * Pos * Rot * Scl;
    mDirtyMatrix = false;
}

void Transformation::ChainTransformation(Transformation *Other)
{
    if (Other != this)
        Chain = Other;
}