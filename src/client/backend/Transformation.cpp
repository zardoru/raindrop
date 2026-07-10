#include "Transformation.h"
//#include <glm/gtc/matrix_transform.inl>

#include <algorithm>

bool Transformation::IsMatrixDirty()
{
	return mDirtyMatrix || Chain;
}

Transformation::Transformation()
{
    SetSize(1);
    SetScale(1);
    SetRotation(0);
    set_position(0, 0);
    Chain = nullptr;
    mLayer = 0;
	mDirtyMatrix = true;

    UpdateMatrix();
}

// Scale
void Transformation::SetScale(const Vec2 Scale)
{
    mScale = Scale;
    mDirtyMatrix = true;
}

void Transformation::SetScale(const float Scale)
{
    SetScaleX(Scale);
    SetScaleY(Scale);
}

void Transformation::SetScaleX(const float ScaleX)
{
    mScale.x = ScaleX;
    mDirtyMatrix = true;
}

void Transformation::SetScaleY(const float ScaleY)
{
    mScale.y = ScaleY;
    mDirtyMatrix = true;
}

Vec2 Transformation::GetScale() const
{
    return mScale;
}

// Position
void Transformation::set_position(const Vec2 Pos)
{
    mPosition = Pos;
    mDirtyMatrix = true;
}

void Transformation::set_position(const float pX, const float pY)
{
    SetPositionX(pX);
    SetPositionY(pY);
}

void Transformation::SetPositionX(const float pX)
{
    mPosition.x = pX;
    mDirtyMatrix = true;
}

void Transformation::SetPositionY(const float pY)
{
    mPosition.y = pY;
    mDirtyMatrix = true;
}

void Transformation::AddPosition(const float pX, const float pY)
{
    mPosition.x += pX;
    mPosition.y += pY;
    mDirtyMatrix = true;
}

void Transformation::AddPosition(const Vec2 pos)
{
    mPosition += pos;
    mDirtyMatrix = true;
}

void Transformation::AddPositionX(const float pX)
{
    AddPosition(pX, 0);
}

void Transformation::AddPositionY(const float pY)
{
    AddPosition(0, pY);
}

Vec2 Transformation::GetPosition() const
{
    return mPosition;
}

// Size
void Transformation::SetSize(const Vec2 Size)
{
    mWidth = Size.x;
    mHeight = Size.y;
    mDirtyMatrix = true;
}

void Transformation::SetSize(const float Size)
{
    SetSize(Vec2(Size, Size));
}

void Transformation::SetSize(const float W, const float H)
{
    SetSize(Vec2(W, H));
}

void Transformation::SetWidth(const float W)
{
    mWidth = W;
    mDirtyMatrix = true;
}

void Transformation::SetHeight(const float H)
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
void Transformation::SetRotation(const float Rot)
{
    mRotation = Rot;
    mDirtyMatrix = true;
}

float Transformation::GetRotation() const
{
    return mRotation;
}

void Transformation::AddRotation(const float Rot)
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

void Transformation::SetZ(const uint32_t Z)
{
    mLayer = std::min(Z, MaxLayer);
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
    Mat4 Scl = glm::scale(glm::identity<Mat4>(), glm::vec3(mWidth*mScale.x, mHeight*mScale.y, 1));
    Mat4 Pos = glm::translate(glm::identity<Mat4>(), glm::vec3(mPosition.x, mPosition.y, mLayer));
    Mat4 Rot = glm::rotate(glm::identity<Mat4>(), mRotation, glm::vec3(0, 0, 1));
    Mat4 Chn = glm::identity<Mat4>();

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
